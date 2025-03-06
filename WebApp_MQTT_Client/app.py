from flask import Flask, render_template, redirect, url_for, request, flash
from flask_sqlalchemy import SQLAlchemy
from flask_login import LoginManager, login_user, login_required, logout_user, current_user, UserMixin
from werkzeug.security import generate_password_hash, check_password_hash
from datetime import datetime
import os
import mqtt_client
import pytz

app = Flask(__name__)

# ======================== CONFIGURATION ========================
# Substitute your own secret key here if not using an environment variable.
# In production, set the environment variable 'SECRET_KEY' on Render.
app.config['SECRET_KEY'] = os.environ.get('SECRET_KEY', 'your-default-secret-key')

# The DATABASE_URL should be set in your Render service settings.
# Ensure it is a complete connection string provided by your PostgreSQL provider (e.g., Supabase).
# For example:
#   postgresql://username:password@actual-host:5432/dbname?sslmode=require
# Do NOT hardcode the credentials in your code.
app.config['SQLALCHEMY_DATABASE_URI'] = os.environ.get('DATABASE_URL', 'postgresql://localhost/webapp_mqtt_client')
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
# ===============================================================

db = SQLAlchemy(app)
login_manager = LoginManager(app)
login_manager.login_view = 'login'

# MODELS
class User(db.Model, UserMixin):
    __tablename__ = "users"
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(80), unique=True, nullable=False)
    password_hash = db.Column(db.String(256), nullable=False)
    role = db.Column(db.String(20), nullable=False, default='user')  # 'admin' or 'user'
    access_start = db.Column(db.Time, nullable=True)
    access_end = db.Column(db.Time, nullable=True)
    profile_pic = db.Column(db.String(120), nullable=True)

    def set_password(self, password):
        self.password_hash = generate_password_hash(password)

    def check_password(self, password):
        return check_password_hash(self.password_hash, password)

class LogEntry(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    user_id = db.Column(db.Integer, nullable=False)
    username = db.Column(db.String(80), nullable=False)
    command = db.Column(db.String(100), nullable=False)
    timestamp = db.Column(db.DateTime, default=datetime.utcnow)

@login_manager.user_loader
def load_user(user_id):
    return User.query.get(int(user_id))

# Before each request: restrict access outside permitted time (for non-admin users)
@app.before_request
def restringir_acesso_por_tempo():
    if current_user.is_authenticated and current_user.role != 'admin':
        if current_user.access_start and current_user.access_end:
            # Adjust timezone as necessary; here we use São Paulo time.
            agora = datetime.now(pytz.timezone("America/Sao_Paulo")).time()
            if not (current_user.access_start <= agora <= current_user.access_end):
                flash("Acesso não permitido neste horário.", "danger")
                logout_user()
                return redirect(url_for('login'))

# ROUTES

# Home page – Controle dos Portões (CCUB)
@app.route('/')
@login_required
def index():
    return render_template('index.html')

# Route to send MQTT command to the gates
@app.route('/gate/<gate_id>/<action>', methods=['POST'])
@login_required
def control_gate(gate_id, action):
    # Only allow 'externo' and 'interno' for gate_id and 'open' or 'close' for action
    if gate_id not in ['externo', 'interno'] or action not in ['open', 'close']:
        flash("Comando inválido", "danger")
        return redirect(url_for('index'))
    
    # Correcting inversion:
    # If the gate is "externo", send to topic for gate3; if "interno", send to topic for gate4
    topic = "gates/gate4/control" if gate_id == 'externo' else "gates/gate3/control"
    
    # Command: "1" to open and "0" to close
    message = "1" if action == "open" else "0"
    
    mqtt_client.publish_message(topic, message)
    
    # Log the command
    log = LogEntry(
        user_id=current_user.id,
        username=current_user.username,
        command=f"Portão {gate_id.capitalize()} {action}",
        timestamp=datetime.utcnow()
    )
    db.session.add(log)
    db.session.commit()
    
    flash(f"Comando '{message}' enviado para o Portão {gate_id.capitalize()}", "success")
    return redirect(url_for('index'))

# Login route
@app.route('/login', methods=['GET', 'POST'])
def login():
    if current_user.is_authenticated:
        return redirect(url_for('index'))
    if request.method == 'POST':
        username = request.form.get('username')
        password = request.form.get('password')
        user = User.query.filter_by(username=username).first()
        if user and user.check_password(password):
            login_user(user)
            flash("Login realizado com sucesso!", "success")
            return redirect(url_for('index'))
        else:
            flash("Usuário ou senha inválidos", "danger")
    return render_template('login.html')

# Logout route
@app.route('/logout')
@login_required
def logout():
    logout_user()
    flash("Você saiu do sistema.", "info")
    return redirect(url_for('login'))

# User profile page
@app.route('/profile', methods=['GET', 'POST'])
@login_required
def profile():
    if request.method == 'POST':
        if 'profile_pic' in request.files:
            file = request.files['profile_pic']
            if file.filename != '':
                from werkzeug.utils import secure_filename
                filename = secure_filename(file.filename)
                filename = f"{current_user.username}_{filename}"
                profile_path = os.path.join('static', 'profile_pics', filename)
                file.save(profile_path)
                current_user.profile_pic = filename
                db.session.commit()
                flash("Foto de perfil atualizada.", "success")
            else:
                flash("Nenhum arquivo selecionado.", "warning")
        else:
            flash("Erro no envio do arquivo.", "warning")
    return render_template('profile.html', user=current_user)

# Administration routes

@app.route('/admin/users', methods=['GET', 'POST'])
@login_required
def manage_users():
    if current_user.role != 'admin':
        flash("Acesso negado.", "danger")
        return redirect(url_for('index'))
    if request.method == 'POST':
        username = request.form.get('username')
        password = request.form.get('password')
        role = request.form.get('role', 'user')
        if User.query.filter_by(username=username).first():
            flash("Usuário já existe.", "warning")
        else:
            new_user = User(username=username, role=role)
            new_user.set_password(password)
            access_start_str = request.form.get('access_start')
            access_end_str = request.form.get('access_end')
            if access_start_str:
                new_user.access_start = datetime.strptime(access_start_str, "%H:%M").time()
            if access_end_str:
                new_user.access_end = datetime.strptime(access_end_str, "%H:%M").time()
            db.session.add(new_user)
            db.session.commit()
            flash("Novo usuário criado.", "success")
    users = User.query.all()
    return render_template('admin_users.html', users=users)

@app.route('/admin/edit_user/<int:user_id>', methods=['GET', 'POST'])
@login_required
def edit_user(user_id):
    if current_user.role != 'admin':
        flash("Acesso negado.", "danger")
        return redirect(url_for('index'))
    user = User.query.get_or_404(user_id)
    if request.method == 'POST':
        user.role = request.form.get('role')
        access_start_str = request.form.get('access_start')
        access_end_str = request.form.get('access_end')
        user.access_start = datetime.strptime(access_start_str, "%H:%M").time() if access_start_str else None
        user.access_end = datetime.strptime(access_end_str, "%H:%M").time() if access_end_str else None
        db.session.commit()
        flash("Usuário atualizado.", "success")
        return redirect(url_for('manage_users'))
    return render_template('edit_user.html', user=user)

@app.route('/admin/delete_user/<int:user_id>', methods=['POST'])
@login_required
def delete_user(user_id):
    if current_user.role != 'admin':
        flash("Acesso negado.", "danger")
        return redirect(url_for('index'))
    user = User.query.get_or_404(user_id)
    if user.username == current_user.username:
        flash("Você não pode deletar seu próprio usuário.", "warning")
        return redirect(url_for('manage_users'))
    db.session.delete(user)
    db.session.commit()
    flash("Usuário deletado.", "success")
    return redirect(url_for('manage_users'))

@app.route('/admin/logs')
@login_required
def view_logs():
    if current_user.role != 'admin':
        flash("Acesso negado.", "danger")
        return redirect(url_for('index'))
    logs = LogEntry.query.order_by(LogEntry.timestamp.desc()).all()
    return render_template('admin_logs.html', logs=logs)

# Route for users to change their own password
@app.route("/change_password", methods=["GET", "POST"], endpoint="user_change_password")
@login_required
def change_password():
    if request.method == "POST":
        new_password = request.form.get("new_password")
        confirm_password = request.form.get("confirm_password")
        if new_password != confirm_password:
            flash("Nova senha e confirmação não conferem.", "danger")
            return redirect(url_for("user_change_password"))
        current_user.set_password(new_password)
        db.session.commit()
        flash("Senha alterada com sucesso!", "success")
        return redirect(url_for("profile"))
    return render_template("change_password.html", admin=False)

# Route for admin to change any user's password (no current password needed)
@app.route("/admin/change_password/<int:user_id>", methods=["GET", "POST"], endpoint="admin_change_password")
@login_required
def admin_change_password(user_id):
    if current_user.role != 'admin':
        flash("Acesso negado.", "danger")
        return redirect(url_for("index"))
    user = User.query.get_or_404(user_id)
    if request.method == "POST":
        new_password = request.form.get("new_password")
        confirm_password = request.form.get("confirm_password")
        if new_password != confirm_password:
            flash("Nova senha e confirmação não conferem.", "danger")
            return redirect(url_for("admin_change_password", user_id=user_id))
        user.set_password(new_password)
        db.session.commit()
        flash("Senha do usuário alterada com sucesso!", "success")
        return redirect(url_for("manage_users"))
    return render_template("change_password.html", admin=True, user=user)

if __name__ == '__main__':
    with app.app_context():
        db.create_all()
        # Create admin user if not already present
        admin = User.query.filter_by(username='admin').first()
        if not admin:
            admin = User(username='admin', role='admin')
            admin.set_password('adminpassword')  # Replace with a secure password
            db.session.add(admin)
            db.session.commit()
            print("Usuário admin criado com usuário 'admin' e senha 'adminpassword'")
    app.run(debug=True)
