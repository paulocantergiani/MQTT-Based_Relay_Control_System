from app import db, app, User
from werkzeug.security import generate_password_hash

with app.app_context():
    db.create_all()
    # Create admin user if it doesn't exist
    admin = User.query.filter_by(username='admin').first()
    if not admin:
        admin = User(username='admin', role='admin')
        admin.set_password('adminpassword')  # Use a secure password in production
        db.session.add(admin)
        db.session.commit()
        print("Admin user created with username 'admin' and password 'adminpassword'")
    else:
        print("Admin user already exists.")
