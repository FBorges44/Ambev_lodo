from flask import Flask
from servidor.rotas import bp  # importa o blueprint do rotas.py

app = Flask(__name__)
app.register_blueprint(bp)    # registra todas as rotas do blueprint

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
