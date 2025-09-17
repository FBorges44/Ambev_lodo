from flask import Flask, render_template, request, jsonify
from flask_socketio import SocketIO
from flask_cors import CORS
import logging

# Desativa o logging padrão do Flask para um terminal mais limpo
log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)

app = Flask(__name__)
CORS(app)
socketio = SocketIO(app, cors_allowed_origins="*")

# Rota principal que carrega nossa página web
@app.route('/')
def index():
    return render_template('index.html')

# Rota que vai receber os dados do Pico W via HTTP POST
@app.route('/cor', methods=['POST'])
def receber_dados_cor():
    if not request.is_json:
        print("❌ Erro: Requisição recebida não estava no formato JSON.")
        return jsonify({"erro": "O corpo da requisicao precisa ser JSON"}), 400

    dados = request.get_json()
    
    # Imprime os dados no terminal para sabermos que funcionou
    print("----------------------------------------")
    print(f"✅ Dados recebidos do Pico W:")
    print(f"   Cor: {dados.get('cor', 'N/A')}")
    print(f"   R: {dados.get('r', 'N/A')}, G: {dados.get('g', 'N/A')}, B: {dados.get('b', 'N/A')}")
    print(f"   Clear: {dados.get('clear', 'N/A')}")
    
    # Envia os dados em tempo real para todos os navegadores conectados
    socketio.emit('update_data', dados)
    
    # Retorna uma resposta de sucesso para o Pico W
    return jsonify({"status": "dados recebidos com sucesso"}), 200

if __name__ == '__main__':
    print("================================================")
    print("🚀 Servidor de Monitoramento de Lodo Iniciado")
    print("   - Abra seu navegador em http://127.0.0.1:5000")
    print("   - O servidor está visível na rede para o Pico W.")
    print("   - Pressione CTRL+C para parar o servidor.")
    print("================================================")
    socketio.run(app, host='0.0.0.0', port=5000)