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


### To Do
- wrong file (missing ';')

- throw quand erreur dans le parsing
- error_page (code limités) 

- Clean code (opti, consistency, static, etc)
- Nettoyer en retirant //// TEMP

### Questions
- si methods vide (déjà géré par Request man)