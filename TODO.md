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


### To Do
- fix no server block (commentaire dans no_server_block.conf pris en compte)
- wrong file (missing ';')
- gérer les { retour de ligne
- throw quand erreur dans le parsing
- check cgi (extension valide) -- nginx
- error_page (code limités) 
- Nettoyer en retirant //// TEMP

### Questions
- si methods vide, on autorise tout ou on stop ?