from flask import Blueprint, request, jsonify
from influxdb_client import InfluxDBClient, Point, WritePrecision

# Criando o blueprint
bp = Blueprint("rotas", __name__)

# Configuração do InfluxDB
URL = "http://localhost:8086"  # endereço do seu InfluxDB
TOKEN = "SEU_TOKEN"            # token de autenticação
ORG = "SEU_ORG"                # sua organização no Influx
BUCKET = "sensores"            # bucket onde os dados serão armazenados

client = InfluxDBClient(url=URL, token=TOKEN, org=ORG)
write_api = client.write_api()
query_api = client.query_api()

# ===========================
# Rota POST - Receber cor
# ===========================
@bp.route("/cor", methods=["POST"])
def receber_cor():
    if not request.is_json:
        return jsonify({"error": "JSON esperado"}), 400

    data = request.get_json()
    cor = data.get("cor")

    if not cor:
        return jsonify({"error": "Campo 'cor' ausente"}), 400

    # Grava no InfluxDB
    point = Point("leituras").tag("tipo", "cor").field("valor", cor)
    write_api.write(bucket=BUCKET, org=ORG, record=point)

    return jsonify({"status": "ok", "cor": cor})

# ===========================
# Rota GET - Consultar últimas cores
# ===========================
@bp.route("/dados", methods=["GET"])
def consultar_dados():
    # Consulta últimas 10 leituras no InfluxDB
    query = f'''
    from(bucket: "{BUCKET}")
      |> range(start: -10m)
      |> filter(fn: (r) => r._measurement == "leituras" and r.tipo == "cor")
      |> limit(n: 10)
    '''
    result = query_api.query(org=ORG, query=query)

    saida = []
    for table in result:
        for record in table.records:
            saida.append({
                "cor": record.get_value(),
                "hora": str(record.get_time())
            })

    return jsonify(saida)
