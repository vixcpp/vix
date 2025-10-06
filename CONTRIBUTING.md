# 🤝 Contribuer à Softadastra

Bienvenue dans le projet **Softadastra**, une marketplace africaine ambitieuse et innovante.  
Ce fichier vous guide pour contribuer de manière propre et efficace.

---

## 🧠 Avant de commencer

- Assurez-vous d’avoir lu le fichier `README.md`
- Vérifiez que votre contribution correspond à la **vision du projet**
- Si vous avez une idée, créez une **Issue GitHub** pour en discuter

---

## 🛠️ Stack technique

- **Frontend** : PHP, HTML, CSS, JS, Bootstrap, Select2, NProgress
- **Backend** : C++ (Crow + Boost.Beast)
- **Versioning** : Git (main / staging / develop)

---

## 🔀 Workflow Git (Branche)

1. Travaillez toujours sur la branche `develop`
2. Une fois terminé, créez un **Pull Request vers `staging`**
3. Ne poussez **jamais** directement sur `main` ni `staging`
4. Exemples :
   ```bash
   git checkout develop
   git pull origin develop
   git checkout -b feat/user-auth
   # Code...
   git add .
   git commit -m "✨ Ajout de l’authentification utilisateur"
   git push origin feat/user-auth
   ```
