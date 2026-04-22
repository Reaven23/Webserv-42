# TODO

## Done
- server_name, juste un name car hors scope
- Check: fichier openable 
- Check: fichier vide
- Check: si pas de block server {
- Check: Check des ports
- Check: si doublons de listen
- Check: si upload on sans path
- Check: si ligne hors sujet 
- Pre fill index/root dans location si vide (en prenant celui du server parent)
- Check: missing '}'

- check cgi (extension valide)
- gérer les { retour de ligne
- si pas de {} pour location (cad si juste random "location" dans conf)
- fix no server block (fix commented block server)

- Check: missing ';'
- error page : >= 300 && <= 599
- error page : no path/no code
- declaration "methods" dans config mais : no methods/wrong method
- declaration "redirect" dans config mais : no redirect path
- redirect code seulement 301 & 302
- redirect path start '/' or 'http://' or 'https://' -> géré au 'max', voir avec http handler
- Lot of details

### To Do ?
- location avec même prefix
- doublon error_page 

### Team 


### To Do
- Clean code (opti, consistency, static, etc)
- Nettoyer en retirant //// TEMP