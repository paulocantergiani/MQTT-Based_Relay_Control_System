# app.py (completo corrigido com Redis para rate limiting)
from flask import Flask, render_template, request, jsonify, redirect, url_for, flash
from flask_login import LoginManager, login_user, logout_user, login_required, UserMixin
from flask_wtf import FlaskForm, CSRFProtect
from flask_limiter import Limiter
from flask_limiter.util import get_remote_address
from flask_talisman import Talisman
from wtforms import StringField, PasswordField, SubmitField
from wtforms.validators import DataRequired
from bcrypt import hashpw, gensalt, checkpw
import AWSIoTPythonSDK.MQTTLib as AWSIoTPyMQTT
import os
import logging
from redis import Redis

app = Flask(__name__)
app.secret_key = os.urandom(32)

# Segurança reforçada
csrf = CSRFProtect(app)
Talisman(app, content_security_policy=None, force_https=False)

# Configuração segura de sessões
app.config.update(
    SESSION_COOKIE_SECURE=True,
    SESSION_COOKIE_HTTPONLY=True,
    SESSION_COOKIE_SAMESITE='Lax'
)

# Limitação de tentativas usando Redis
redis_client = Redis(host='localhost', port=6379, db=0)
limiter = Limiter(
    app=app,
    key_func=get_remote_address,
    storage_uri="redis://localhost:6379/0"
)

# Logging ajustado
logging.basicConfig(
    filename='access.log',
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s'
)

# Flask-Login
login_manager = LoginManager()
login_manager.init_app(app)
login_manager.login_view = 'login'

# Usuário de exemplo (substitua por banco real)
class User(UserMixin):
    def __init__(self, id, username, password_hash):
        self.id = id
        self.username = username
        self.password_hash = password_hash

# Banco falso para demonstração
users = {
    "admin": User(id=1, username="admin", password_hash=hashpw(b"admin", gensalt()))
}

@login_manager.user_loader
def load_user(user_id):
    for user in users.values():
        if user.id == int(user_id):
            return user
    return None

class LoginForm(FlaskForm):
    username = StringField('Usuário', validators=[DataRequired()])
    password = PasswordField('Senha', validators=[DataRequired()])
    submit = SubmitField('Entrar')

AWS_IOT_ENDPOINT = ""
CLIENT_ID = "flask-gate-controller"
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
mqtt_client = AWSIoTPyMQTT.AWSIoTMQTTClient(CLIENT_ID)
mqtt_client.configureEndpoint(AWS_IOT_ENDPOINT, 8883)
mqtt_client.configureCredentials(
    os.path.join(BASE_DIR, "certs/AmazonRootCA1.pem"),
    os.path.join(BASE_DIR, "certs/device-private.pem.key"),
    os.path.join(BASE_DIR, "certs/device-certificate.pem.crt")
)
mqtt_client.connect()

TOPIC_MAP = {
    "gate1_on":  "gates/gate1/control",
    "gate1_off": "gates/gate1/control",
    "gate2_on":  "gates/gate2/control",
    "gate2_off": "gates/gate2/control",
}

@app.route('/login', methods=['GET', 'POST'])
@limiter.limit("5/minute")
def login():
    form = LoginForm()
    if form.validate_on_submit():
        user = users.get(form.username.data)
        if user and checkpw(form.password.data.encode(), user.password_hash):
            login_user(user)
            logging.info(f"Usuário {form.username.data} autenticado com sucesso (IP: {request.remote_addr})")
            return redirect(url_for('index'))
        else:
            flash("Credenciais incorretas.")
            logging.warning(f"Falha de login para usuário {form.username.data} (IP: {request.remote_addr})")
    return render_template('login.html', form=form)

@app.route('/logout')
@login_required
def logout():
    logout_user()
    return redirect(url_for('login'))

@app.route("/")
@login_required
def index():
    return render_template("index.html")

@app.route("/toggle_gate", methods=["POST"])
@login_required
@limiter.limit("60/minute")
def toggle_gate():
    data = request.get_json(force=True)
    action = data.get("action")
    if action in TOPIC_MAP:
        topic = TOPIC_MAP[action]
        message = "turn_on" if action.endswith("on") else "turn_off"
        mqtt_client.publish(topic, message, 0)
        logging.info(f'Ação MQTT: {action} realizada pelo IP: {request.remote_addr}')
        return jsonify({"status": "ok", "actionReceived": action})
    logging.error(f'Ação inválida recebida: {action} (IP: {request.remote_addr})')
    return jsonify({"status": "error", "message": "invalid action"}), 400

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)