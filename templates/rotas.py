from flask import Blueprint

# Criando o blueprint
bp = Blueprint("rotas", __name__)

# Rota principal
@bp.route("/")
def index():
    return "Servidor Flask está funcionando via blueprint!"
;