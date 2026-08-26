import os
import json
from datetime import datetime
import ydb

# --- НАСТРОЙКИ ПОДКЛЮЧЕНИЯ К БАЗЕ ДАННЫХ ---
# Вставьте сюда ваши данные из вкладки «Обзор» вашей YDB
YDB_ENDPOINT = "grpcs://ydb.serverless.yandexcloud.net:2135"
YDB_DATABASE = "/ru-central1/b1g9h5hpehdeoa2p8t2g/etnbag3rj74cf18sh6d9"


# Инициализируем драйвер для работы с YDB
# driver = ydb.Driver(endpoint=YDB_ENDPOINT, database=YDB_DATABASE)
driver = ydb.Driver(
    endpoint=YDB_ENDPOINT, 
    database=YDB_DATABASE, 
    credentials=ydb.iam.MetadataUrlCredentials()
)

driver.wait(fail_fast=True, timeout=5)
pool = ydb.SessionPool(driver)

def write_to_ydb(session, sensor_id, timestamp, value):
    # Запрос на добавление данных в таблицу
    query = """
    DECLARE $sensor_id AS String;
    DECLARE $timestamp AS Timestamp;
    DECLARE $value AS Double;

    UPSERT INTO haus_temperature (sensor_id, timestamp, value)
    VALUES ($sensor_id, $timestamp, $value);
    """
    
    # Готовим параметры для безопасной записи (защита от инъекций)
    prepared_query = session.prepare(query)
    session.transaction(ydb.SerializableReadWrite()).execute(
        prepared_query,
        {
            "$sensor_id": sensor_id.encode('utf-8'), # Строки в YDB передаются как байты
            "$timestamp": int(timestamp.timestamp() * 1000000), # Переводим время в микросекунды для YDB
            "$value": float(value)
        },
        commit_tx=True,
    )

def handler(event, context):
    try:
        # 1. Извлекаем JSON-тело, которое прислала Ардуинка
        body = json.loads(event['body'])
        sensor_id = body['sensor_id']
        value = body['value']
        
        # 2. Автоматически берем текущее серверное время Яндекса
        current_time = datetime.now()
        
        # 3. Записываем данные в базу YDB
        pool.retry_operation_sync(write_to_ydb, sensor_id=sensor_id, timestamp=current_time, value=value)
        
        # 4. Отвечаем Ардуинке, что всё прошло успешно
        return {
            'statusCode': 200,
            'headers': {'Content-Type': 'application/json'},
            'body': json.dumps({'status': 'success', 'saved_at': str(current_time)})
        }
        
    except Exception as e:
        # Если что-то пошло не так (например, Ардуинка прислала кривой JSON)
        return {
            'statusCode': 400,
            'body': json.dumps({'status': 'error', 'message': str(e)})
        }
