# Test de transport binaire avant lot 1790

Avant toute pousse du lot 1790, un test non attaché à l'arbre Git est autorisé pour vérifier si le connecteur GitHub sait accepter directement un chemin de fichier local dans `create_blob`. Aucun ref/commit/branche ne sera modifié par ce test. Si le connecteur ne supporte pas ce mode, le blob orphelin éventuel sera ignoré et aucune donnée de production ne sera affectée.
