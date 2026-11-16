#include "MemsDocumentaryRepository.h"
#include "../expert/IaMemsConversationRouting.h"
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTemporaryDir>
#include <QVariantMap>
#include <cstdio>

static bool require(bool condition,const char* reason)
{
    if(!condition)
        std::fprintf(stderr,"MEMS_NATIVE_INTENT_SELFTEST_FAILED: %s\n",reason);
    return condition;
}

int main(int argc,char** argv)
{
    QCoreApplication app(argc,argv);
    QTemporaryDir dir;
    if(!require(dir.isValid(),"temporary root"))return 1;
    const QString path=QDir(dir.path()).filePath(QStringLiteral("native_fr.sqlite"));
    {
        QSqlDatabase db=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                                  QStringLiteral("native_intent_seed"));
        db.setDatabaseName(path);
        if(!require(db.open(),"seed sqlite"))return 1;
        const QStringList schema={
            QStringLiteral("CREATE TABLE documentary_database("
                           "singleton INTEGER PRIMARY KEY,schema_version INTEGER,"
                           "language TEXT,publication_status TEXT)"),
            QStringLiteral("CREATE TABLE documentary_documents("
                           "document_id TEXT,source_revision TEXT,language TEXT)"),
            QStringLiteral("CREATE TABLE documentary_pages("
                           "document_id TEXT,physical_page INTEGER)"),
            QStringLiteral("CREATE TABLE documentary_images("
                           "document_id TEXT,physical_page INTEGER,relative_path TEXT)"),
            QStringLiteral("CREATE TABLE documentary_entries("
                           "document_id TEXT,source_revision TEXT,entry_id TEXT,"
                           "entry_type TEXT,source_page INTEGER,title TEXT,"
                           "source_text TEXT,verification_status TEXT,"
                           "source_table TEXT,raw_json TEXT)"),
            QStringLiteral("CREATE VIRTUAL TABLE documentary_fts USING fts5("
                           "title,source_text,content='documentary_entries',"
                           "content_rowid='rowid',"
                           "tokenize='unicode61 remove_diacritics 2')"),
            QStringLiteral("INSERT INTO documentary_database VALUES(1,1,'fr','source_inventory')"),
            QStringLiteral("INSERT INTO documentary_documents VALUES('RCL0193FRE','v1','fr')"),
            QStringLiteral("INSERT INTO documentary_pages VALUES('RCL0193FRE',59)"),
            QStringLiteral("INSERT INTO documentary_pages VALUES('RCL0193FRE',60)"),
            QStringLiteral("INSERT INTO documentary_pages VALUES('RCL0193FRE',42)"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','mems_procedure:1','procedure',59,"
                           "'JOINT DE CULASSE - dépose',"
                           "'title_source: JOINT DE CULASSE - dépose\\n"
                           "Opération de réparation N° 12.29.02; début PDF p.59',"
                           "'source_checked','mems_procedure','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','mems_procedure:2','procedure',60,"
                           "'JOINT DE CULASSE - repose',"
                           "'title_source: JOINT DE CULASSE - repose\\n"
                           "Opération de réparation N° 12.29.02; début PDF p.60',"
                           "'source_checked','mems_procedure','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','mems_knowledge_item:1','reference',42,"
                           "'culasse écrous séquence de serrage',"
                           "'source_text: Écrous de culasse : 34 N.m puis 34 N.m de plus',"
                           "'source_checked','mems_knowledge_item','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','p3','procedure',70,"
                           "'MAITRE CYLINDRE DE FREIN - réparation',"
                           "'Consignes constructeur MAITRE CYLINDRE DE FREIN - réparation page 70',"
                           "'source_checked','mems_procedure','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','p4','procedure',71,"
                           "'MAITRE CYLINDRE DE FREIN - contrôle',"
                           "'Consignes constructeur MAITRE CYLINDRE DE FREIN - contrôle page 71',"
                           "'source_checked','mems_procedure','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','p5','procedure',72,"
                           "'MAITRE CYLINDRE DE FREIN - réglage',"
                           "'Consignes constructeur MAITRE CYLINDRE DE FREIN - réglage page 72',"
                           "'source_checked','mems_procedure','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','p6','procedure',73,"
                           "'MAITRE CYLINDRE DE FREIN - remplacement',"
                           "'Consignes constructeur MAITRE CYLINDRE DE FREIN - remplacement page 73',"
                           "'source_checked','mems_procedure','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','p7','procedure',74,"
                           "'MAITRE CYLINDRE DE FREIN - purge',"
                           "'Consignes constructeur MAITRE CYLINDRE DE FREIN - purge page 74',"
                           "'source_checked','mems_procedure','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','p8','procedure',75,"
                           "'MAITRE CYLINDRE DE FREIN - dépose',"
                           "'Consignes constructeur MAITRE CYLINDRE DE FREIN - dépose page 75',"
                           "'source_checked','mems_procedure','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','p9','procedure',76,"
                           "'MAITRE CYLINDRE DE FREIN - repose',"
                           "'Consignes constructeur MAITRE CYLINDRE DE FREIN - repose page 76',"
                           "'source_checked','mems_procedure','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','motor','procedure',77,"
                           "'MOTEUR ET BOITE - dépose',"
                           "'Débrancher les tuyaux de frein pendant la dépose du moteur',"
                           "'source_checked','mems_procedure','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','piston-mention','reference',251,"
                           "'ETRIER DE FREIN AVANT - inspection',"
                           "'Le cylindre contient deux pistons. Vérifier la liberté de déplacement.',"
                           "'source_checked','mems_knowledge_item','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','injecteur-ref','reference',112,"
                           "'Injecteur',"
                           "'Injecteur: fonctionnement et commande du solénoïde.',"
                           "'source_checked','mems_knowledge_item','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','injecteur-location','reference',113,"
                           "'Emplacement : Injecteur',"
                           "'Repérage des injecteurs sur la rampe d alimentation.',"
                           "'source_checked','mems_knowledge_item','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','iat-incidental','reference',114,"
                           "'IAT température admission injecteurs carburant',"
                           "'Le calculateur ajuste les injecteurs suivant la sonde IAT.',"
                           "'source_checked','mems_knowledge_item','{}')"),
            QStringLiteral("INSERT INTO documentary_fts(documentary_fts) VALUES('rebuild')")
        };
        for(const QString& sql:schema){
            QSqlQuery query(db);
            if(!require(query.exec(sql),"fixture schema and data"))return 1;
        }
        // Property-style coverage: insert several arbitrary subjects with
        // two operations and one indirect mention. No engine code branches
        // on any of these example words.
        const QStringList genericParts={
            QStringLiteral("rotule"), QStringLiteral("palier"),
            QStringLiteral("poulie"), QStringLiteral("tendeur"),
            QStringLiteral("charniere"), QStringLiteral("conduit")
        };
        QSqlQuery genericInsert(db);
        genericInsert.prepare(QStringLiteral(
            "INSERT INTO documentary_entries VALUES(?,?,?,?,?,?,?,?,?,?)"));
        for(int p=0;p<genericParts.size();++p){
            const QString component=genericParts.at(p);
            for(int operation=0;operation<2;++operation){
                const QString action=operation==0
                    ?QStringLiteral("dépose"):QStringLiteral("repose");
                const QString id=QStringLiteral("generic-%1-%2").arg(p).arg(operation);
                genericInsert.addBindValue(QStringLiteral("RCL0193FRE"));
                genericInsert.addBindValue(QStringLiteral("v1"));
                genericInsert.addBindValue(id);
                genericInsert.addBindValue(QStringLiteral("procedure"));
                genericInsert.addBindValue(400+p);
                genericInsert.addBindValue(QStringLiteral("%1 - %2")
                    .arg(component.toUpper(),action));
                genericInsert.addBindValue(QStringLiteral("Consignes vérifiées pour %1 %2.")
                    .arg(component,action));
                genericInsert.addBindValue(QStringLiteral("source_checked"));
                genericInsert.addBindValue(QStringLiteral("mems_procedure"));
                genericInsert.addBindValue(QStringLiteral("{}"));
                if(!require(genericInsert.exec(),"insert generic workshop operation"))return 1;
            }
            genericInsert.addBindValue(QStringLiteral("RCL0193FRE"));
            genericInsert.addBindValue(QStringLiteral("v1"));
            genericInsert.addBindValue(QStringLiteral("generic-mention-%1").arg(p));
            genericInsert.addBindValue(QStringLiteral("step"));
            genericInsert.addBindValue(450+p);
            genericInsert.addBindValue(QStringLiteral("AUTRE ENSEMBLE - étape"));
            genericInsert.addBindValue(QStringLiteral(
                "Le composant %1 est mentionné dans une procédure différente.")
                    .arg(component));
            genericInsert.addBindValue(QStringLiteral("source_checked"));
            genericInsert.addBindValue(QStringLiteral("mems_procedure_step"));
            genericInsert.addBindValue(QStringLiteral("{}"));
            if(!require(genericInsert.exec(),"insert generic indirect mention"))return 1;
        }
        QSqlQuery rebuild(db);
        if(!require(rebuild.exec(QStringLiteral(
             "INSERT INTO documentary_fts(documentary_fts) VALUES('rebuild')")),
             "rebuild generic FTS corpus"))return 1;
        db.close();
        db=QSqlDatabase();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("native_intent_seed"));
    QSettings settings(QSettings::IniFormat,QSettings::UserScope,
                       QStringLiteral("ECU_Mems_Manager"));
    settings.beginGroup(QStringLiteral("Settings"));
    settings.setValue(QStringLiteral("Language"),QStringLiteral("fr"));
    settings.endGroup();
    qputenv("MEMS_DOCUMENTARY_DB",path.toUtf8());
    MemsDocumentaryRepository repository;
    if(!require(repository.open(),"reader accepts native corpus"))return 1;

    const QStringList removalQuestions={
        QStringLiteral("Donne-moi la procédure de dépose du joint de culasse"),
        QStringLiteral("Explique-moi comment faire la dépose du joint de culasse"),
        QStringLiteral("Quelles sont les étapes pour déposer le joint de culasse ?"),
        QStringLiteral("dépose joint culasse")
    };
    for(const QString& question:removalQuestions){
        const QVariantList found=repository.search(question,12,true,QStringLiteral("fr"));
        if(found.isEmpty()) {
            std::fprintf(stderr,"NATIVE_QUERY_NO_HITS_FOR=%s\\n",qPrintable(question));
            QSqlDatabase debug=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),QStringLiteral("native-debug"));
            debug.setDatabaseName(path);
            if(debug.open()) {
                QSqlQuery fts(debug);
                fts.prepare(QStringLiteral("SELECT COUNT(*) FROM documentary_fts WHERE documentary_fts MATCH ?"));
                fts.addBindValue(QStringLiteral("\"joint\" AND \"culasse\""));
                const bool executed=fts.exec();
                const QString diagnostic=fts.lastError().text();
                std::fprintf(stderr,"NATIVE_PROBE_EXEC=%d ERROR=%s\\n",
                             int(executed),qPrintable(diagnostic));
                if(fts.next())std::fprintf(stderr,"NATIVE_PROBE_COUNT=%d\\n",fts.value(0).toInt());
                QSqlQuery all(debug);
                if(all.exec(QStringLiteral("SELECT COUNT(*) FROM documentary_fts")) && all.next())
                    std::fprintf(stderr,"NATIVE_FTS_ALL_ROWS=%d\\n",all.value(0).toInt());
                debug.close();
            }
            debug=QSqlDatabase();
            QSqlDatabase::removeDatabase(QStringLiteral("native-debug"));
            return 1;
        }
        const QVariantMap first=found.first().toMap();
        if(!require(first.value(QStringLiteral("title")).toString().contains(
             QStringLiteral("dépose")),"removal requested must prioritize removal"))return 1;
        if(!require(first.value(QStringLiteral("source_page")).toInt()==59,
                    "removal original source page 59"))return 1;
        if(!require(!found.first().toMap().value(QStringLiteral("source_key")).toString().isEmpty(),
                    "original source identity kept"))return 1;
    }
    const QStringList refitQuestions={
        QStringLiteral("Donne-moi la procédure de repose du joint de culasse"),
        QStringLiteral("Comment remonter le joint de culasse ?")
    };
    for(const QString& question:refitQuestions){
        const QVariantList found=repository.search(question,12,true,QStringLiteral("fr"));
        if(!require(!found.isEmpty() &&
                found.first().toMap().value(QStringLiteral("title")).toString()
                    .contains(QStringLiteral("repose")),
                    "refit requested must prioritize refit"))return 1;
        if(!require(found.first().toMap().value(QStringLiteral("source_page")).toInt()==60,
                    "refit original source page 60"))return 1;
    }
    const QVariantList direct=repository.search(QStringLiteral("joint de culasse"));
    if(!require(direct.size()>=2,"general topic returns both real operations"))return 1;
    // A broad component query must offer every real workshop operation,
    // including repairs, controls, settings, replacement and bleeding.
    const QVariantList brakeOffers=repository.search(
        QStringLiteral("maitre cylindre de frein"),100,true,QStringLiteral("fr"));
    const QString menu=MemsDocumentaryRepository::numberedMenu(
        QStringLiteral("maitre cylindre de frein"),brakeOffers,true);
    for (const QString &kind : {QStringLiteral("réparation"), QStringLiteral("contrôle"),
            QStringLiteral("réglage"), QStringLiteral("remplacement"),
            QStringLiteral("purge"), QStringLiteral("dépose"), QStringLiteral("repose")}) {
        if(!require(menu.contains(kind),"workshop subject lists all actual operations"))return 1;
    }
    const QVariantList repair=repository.search(
        QStringLiteral("réparation du maitre cylindre de frein"),100,true,QStringLiteral("fr"));
    if(!require(!repair.isEmpty() &&
                repair.first().toMap().value(QStringLiteral("title")).toString()
                    .contains(QStringLiteral("MAITRE CYLINDRE")),
                "repair request keeps the component without forcing removal/refit"))return 1;
    const QVariantList control=repository.search(
        QStringLiteral("contrôle maitre cylindre de frein"),100,true,QStringLiteral("fr"));
    if(!require(!control.isEmpty() &&
                control.first().toMap().value(QStringLiteral("title")).toString()
                    .contains(QStringLiteral("MAITRE CYLINDRE")),
                "control request stays with the intended brake component"))return 1;
    const QVariantList review=repository.search(
        QStringLiteral("révision du maitre cylindre de frein"),100,true,QStringLiteral("fr"));
    if(!require(!review.isEmpty(),
                "workshop operation word must not block otherwise available procedures"))return 1;
    const QVariantList unrelated=repository.search(QStringLiteral("reprogrammer le vaisseau spatial"));
    if(!require(unrelated.isEmpty(),"unrelated question must not return an index"))return 1;
    // Regression: "PISTON" occurs in a checked passage without being the
    // heading. Offer it as a clearly labelled documentary mention, rather
    // than inventing a piston removal procedure or returning no result.
    const QVariantList pistonHits=repository.search(
        QStringLiteral("PISTON"),100,true,QStringLiteral("fr"));
    const QVariantList pistonTopics=MemsDocumentaryRepository::relevantChoices(
        QStringLiteral("PISTON"),pistonHits);
    if(!require(!pistonTopics.isEmpty() &&
                pistonTopics.first().toMap().value(
                    QStringLiteral("iaMemsMentionOnly")).toBool(),
                "piston mentioned only in text must be offered as a mention"))return 1;
    const QString pistonMenu=MemsDocumentaryRepository::numberedMenu(
        QStringLiteral("PISTON"),pistonTopics,true);
    if(!require(pistonMenu.contains(QStringLiteral("mention dans le document")),
                "source mention must be unambiguously labelled"))return 1;
    const QString pistonAnswer=MemsDocumentaryRepository::answerFromHits(
        QVariantList{pistonTopics.first()});
    if(!require(pistonAnswer.contains(QStringLiteral("pistons")) &&
                pistonAnswer.contains(QStringLiteral("page 251")) &&
                pistonAnswer.contains(QStringLiteral("pas d'une procédure")),
                "piston mention selection must preserve source, excerpt and scope"))return 1;

    // Regression: the IAT is a different topic even though its heading
    // incidentally includes the word "injecteurs".
    const QVariantList injectorHits=repository.search(
        QStringLiteral("INJECTEUR"),100,true,QStringLiteral("fr"));
    const QVariantList injectorTopics=MemsDocumentaryRepository::relevantChoices(
        QStringLiteral("INJECTEUR"),injectorHits);
    if(!require(injectorTopics.size()==2,
                "injector query must keep the two dedicated headings only"))return 1;
    for(const QVariant &entry:injectorTopics)
        if(!require(!entry.toMap().value(QStringLiteral("title"))
                 .toString().contains(QStringLiteral("IAT")),
                 "IAT description must not be an injector operation"))return 1;
    const QVariantList allBrakeTopics=MemsDocumentaryRepository::relevantChoices(
        QStringLiteral("FREIN"),
        repository.search(QStringLiteral("FREIN"),100,true,QStringLiteral("fr")));
    if(!require(allBrakeTopics.size()>=7,
                "broad brake subject must retain all manufacturer operations"))return 1;

    // Reproduce the brake workshop conversation: a numbered documentary
    // selection must retain its three source phases and original page, and
    // must never append an engine-specific warning to a brake procedure.
    QVariantMap selectedBrake;
    selectedBrake.insert(QStringLiteral("source_key"),
                         QStringLiteral("rave-fr:RCL0193FRE:brake-operation"));
    selectedBrake.insert(QStringLiteral("source_page"),247);
    selectedBrake.insert(QStringLiteral("source_table"),
                         QStringLiteral("rover_fr_mems_knowledge_item"));
    selectedBrake.insert(QStringLiteral("source_status"),QStringLiteral("source_checked"));
    selectedBrake.insert(QStringLiteral("title"),
                         QStringLiteral("SEGMENTS DE FREIN ARRIERE - operation 70.40.03"));
    selectedBrake.insert(QStringLiteral("content"),QStringLiteral(
        "document : RCL0193FRE\nlangue : francais\npage physique : 247\n\n"
        "Dépose\nAVERTISSEMENT: Remplacer tous les segments du même essieu.\n"
        "1. Déposer les deux tambours.\n"
        "4. Dégager le ressort du segment secondaire.\n"
        "FREINS\nREPARATIONS\n11\n"
        "5. Enlever les ressorts.\n"
        "6. Maintenir les pistons avec un élastique.\n"
        "Inspection\n1. Nettoyer le flasque.\n"
        "2. Vérifier l'état des ressorts.\n"
        "Repose\n1. Lubrifier les points de contact.\n"
        "10. Poser les freins arrière à tambour.\n"
        "12\nENSEMBLE DE SERVOCOMMANDE\n"
        "notes: Références côté gauche/droit conservées ensemble."));
    const QString brakeReply=MemsDocumentaryRepository::answerFromHits(
        QVariantList{selectedBrake});
    if(!require(brakeReply.contains(QStringLiteral("Dépose")) &&
                brakeReply.contains(QStringLiteral("Inspection")) &&
                brakeReply.contains(QStringLiteral("Repose")),
                "the selected brake source must preserve all three phases"))return 1;
    if(!require(brakeReply.contains(QStringLiteral("RCL0193FRE, page 247")),
                "selected operation must preserve manufacturer source and page"))return 1;
    if(!require(!brakeReply.contains(QStringLiteral("variante du moteur")),
                "brake answers must not contain an unrelated engine warning"))return 1;
    if(!require(brakeReply.contains(QStringLiteral("AVERTISSEMENT:")) &&
                brakeReply.contains(QStringLiteral("5. Enlever les ressorts.")) &&
                brakeReply.contains(QStringLiteral("10. Poser les freins")),
                "layout cleanup must never discard real repair steps or warnings"))return 1;
    if(!require(!brakeReply.contains(QStringLiteral("\nFREINS\n")) &&
                !brakeReply.contains(QStringLiteral("\nREPARATIONS\n")) &&
                !brakeReply.contains(QStringLiteral("ENSEMBLE DE SERVOCOMMANDE")) &&
                !brakeReply.contains(QStringLiteral("Références côté gauche/droit")),
                "PDF running headings, next chapter and editorial notes must not leak into operation"))return 1;
    // A value on its own line is NOT a page number without the adjacent
    // manufacturer heading. Protect such values against over-cleaning.
    QVariantMap standaloneValue=selectedBrake;
    standaloneValue.insert(QStringLiteral("content"),QStringLiteral(
        "document : RCL0193FRE\nlangue : francais\npage physique : 247\n\n"
        "Contrôle\n1. Mesurer le jeu.\n12\n2. Répéter la mesure."));
    const QString valueReply=MemsDocumentaryRepository::answerFromHits(
        QVariantList{standaloneValue});
    if(!require(valueReply.contains(QStringLiteral("\n12\n")),
                "a standalone technical value must remain without page-heading proof"))return 1;

    // Layout must also be removed before an explanatory (unnumbered)
    // paragraph and at the tail of a numbered operation.
    QVariantMap iatExcerpt=selectedBrake;
    iatExcerpt.insert(QStringLiteral("title"),
        QStringLiteral("IAT - description constructeur"));
    iatExcerpt.insert(QStringLiteral("content"),QStringLiteral(
        "document : RCL0193FRE\nlangue : français\npage physique : 112\n\n"
        "SYSTEME DE GESTION MOTEUR\nDESCRIPTION ET FONCTIONNEMENT\n7\n"
        "La sonde de température d'admission (IAT) est du type NTC.\n"
        "Le module ECM utilise ce signal pour doser le carburant."));
    const QString iatAnswer=MemsDocumentaryRepository::answerFromHits(
        QVariantList{iatExcerpt});
    if(!require(iatAnswer.contains(QStringLiteral("La sonde de température")) &&
                !iatAnswer.contains(QStringLiteral("SYSTEME DE GESTION MOTEUR")) &&
                !iatAnswer.contains(QStringLiteral("DESCRIPTION ET FONCTIONNEMENT")) &&
                !iatAnswer.contains(QStringLiteral("\n7\n")),
                "opening PDF headers must not contaminate IAT description"))return 1;
    QVariantMap switchExcerpt=selectedBrake;
    switchExcerpt.insert(QStringLiteral("title"),
        QStringLiteral("CONTACTEUR DE TEMOIN DE DEFAILLANCE DE FREIN"));
    switchExcerpt.insert(QStringLiteral("content"),QStringLiteral(
        "document : RCL0193FRE\nlangue : français\npage physique : 239\n\n"
        "Dépose\n1. Débrancher les connecteurs.\n"
        "Repose\n1. Poser l'interrupteur.\n"
        "FREINS\nREPARATIONS"));
    const QString switchAnswer=MemsDocumentaryRepository::answerFromHits(
        QVariantList{switchExcerpt});
    if(!require(switchAnswer.contains(QStringLiteral("Poser l'interrupteur")) &&
                !switchAnswer.contains(QStringLiteral("\nFREINS\nREPARATIONS")),
                "trailing PDF running headings must be trimmed"))return 1;

    // Generic invariants (not per-keyword exceptions):
    // 1. Every named request replaces context. Only an operation-only followup
    //    is attached to the currently selected component.
    const QStringList arbitrarySubjects={
        QStringLiteral("rotule"),QStringLiteral("palier"),
        QStringLiteral("poulie"),QStringLiteral("tendeur"),
        QStringLiteral("charniere"),QStringLiteral("conduit")
    };
    const QStringList operationWords={
        QStringLiteral("dépose"),QStringLiteral("repose"),
        QStringLiteral("réparation"),QStringLiteral("contrôle"),
        QStringLiteral("réglage"),QStringLiteral("purge"),
        QStringLiteral("remplacement"),QStringLiteral("révision")
    };
    for(const QString &oldTopic:arbitrarySubjects){
        for(const QString &newTopic:arbitrarySubjects){
            if(newTopic==oldTopic)
                continue;
            if(!require(IaMemsConversationRouting::contextualWorkshopQuestion(
                    oldTopic,newTopic)==newTopic,
                "any new named component replaces the previous subject"))return 1;
            if(!require(IaMemsConversationRouting::contextualWorkshopQuestion(
                    oldTopic,newTopic+QStringLiteral(" dépose"))
                        ==newTopic+QStringLiteral(" dépose"),
                "named workshop requests never inherit the previous topic"))return 1;
        }
        for(const QString &action:operationWords){
            if(!require(IaMemsConversationRouting::contextualWorkshopQuestion(
                    oldTopic,action)==oldTopic+QLatin1Char(' ')+action,
                "all bare workshop operations preserve the current subject"))return 1;
        }
        // 2. The database search retrieves varied operations, and title
        //    relevance never elevates an unrelated source-text mention.
        const QVariantList searchHits=repository.search(oldTopic,100,true,
                                                         QStringLiteral("fr"));
        const QVariantList chosen=MemsDocumentaryRepository::relevantChoices(
            oldTopic,searchHits);
        if(!require(chosen.size()==2,
                    "each arbitrary component retains its real operation titles"))return 1;
        for(const QVariant &value:chosen){
            const QVariantMap row=value.toMap();
            if(!require(row.value(QStringLiteral("title")).toString().toLower()
                              .contains(oldTopic) &&
                        !row.value(QStringLiteral("iaMemsMentionOnly")).toBool(),
                "direct procedure titles outrank incidental source mentions"))return 1;
        }
        // 3. With no directly named operation, source mentions are still
        //    retrievable and clearly marked, including indexed step entries.
        QVariantList indirectOnly;
        for(const QVariant &value:searchHits)
            if(value.toMap().value(QStringLiteral("entry_type")).toString()
               ==QStringLiteral("step"))
                indirectOnly.append(value);
        const QVariantList indirect=MemsDocumentaryRepository::relevantChoices(
            oldTopic,indirectOnly);
        if(!require(!indirect.isEmpty() &&
                    indirect.first().toMap().value(
                        QStringLiteral("iaMemsMentionOnly")).toBool(),
                "indirect workshop step is a labelled mention, not a procedure"))return 1;
        const QString mentionAnswer=MemsDocumentaryRepository::answerFromHits(
            QVariantList{indirect.first()});
        if(!require(mentionAnswer.contains(QStringLiteral("page")) &&
                    mentionAnswer.contains(QStringLiteral("pas d'une procédure")),
                "indirect source evidence retains page without inventing an operation"))return 1;

        // 4. Duplicate display headings collapse in the first menu, while
        //    every independent documentary source remains selectable.
        QVariantList duplicates;
        for(int index=0;index<5;++index){
            QVariantMap item=chosen.first().toMap();
            item.insert(QStringLiteral("source_key"),
                QStringLiteral("synthetic:%1:%2").arg(oldTopic).arg(index));
            item.insert(QStringLiteral("source_page"),600+index);
            duplicates.append(item);
        }
        duplicates.append(chosen.last());
        const QVariantList grouped=MemsDocumentaryRepository::groupedChoices(
            duplicates);
        if(!require(grouped.size()==2 &&
                    grouped.first().toMap().value(
                        QStringLiteral("iaMemsGroupRows")).toList().size()==5,
                "repeated headings group without discarding distinct source rows"))return 1;
        const QVariantList members=grouped.first().toMap()
            .value(QStringLiteral("iaMemsGroupRows")).toList();
        QSet<QString> retained;
        for(const QVariant &member:members)
            retained.insert(member.toMap().value(QStringLiteral("source_key")).toString());
        if(!require(retained.size()==5,
                    "grouped references preserve all original document identities"))return 1;
        const QString groupedMenu=MemsDocumentaryRepository::numberedMenu(
            oldTopic,grouped,true);
        if(!require(groupedMenu.contains(QStringLiteral("5 références")) &&
                    groupedMenu.contains(QStringLiteral("1. ")) &&
                    groupedMenu.contains(QStringLiteral("2. ")),
                "lossless groups are clearly identified in the numbered menu"))return 1;
    }
    // 5. Source identifiers must not leak as technical field names, while
    //    the raw source remains unchanged for the selected row.
    QVariantMap technical;
    technical.insert(QStringLiteral("title"),QStringLiteral(
        "torque_moteur_support_de_volant_boulons torque"));
    technical.insert(QStringLiteral("source_key"),QStringLiteral("immutable-source-key"));
    technical.insert(QStringLiteral("entry_type"),QStringLiteral("technical_value"));
    const QString technicalMenu=MemsDocumentaryRepository::numberedMenu(
        QStringLiteral("composant"),QVariantList{technical},true);
    if(!require(technicalMenu.contains(QStringLiteral("Couple de serrage")) &&
                !technicalMenu.contains(QStringLiteral("torque_")) &&
                !technicalMenu.contains(QStringLiteral("_de_")) &&
                technical.value(QStringLiteral("title")).toString().startsWith(
                    QStringLiteral("torque_")),
                "technical slug is formatted only for display, source left intact"))return 1;
    // General page-layout invariants across varied component names and
    // step counts. This tests the rendering contract rather than adding
    // per-component special cases to the production parser.
    for(const QString &component:arbitrarySubjects){
        QVariantMap page;
        page.insert(QStringLiteral("source_key"),QStringLiteral(
            "rave-fr:RCL0193FRE:format-test"));
        page.insert(QStringLiteral("document_id"),QStringLiteral("RCL0193FRE"));
        page.insert(QStringLiteral("source_page"),180);
        page.insert(QStringLiteral("source_table"),
                    QStringLiteral("rover_fr_mems_knowledge_item"));
        page.insert(QStringLiteral("source_status"),QStringLiteral("source_checked"));
        page.insert(QStringLiteral("title"),component.toUpper());
        const QString original=QStringLiteral(
            "document : RCL0193FRE\nlangue : français\n"
            "page physique : 180\n\n"
            "Opération de réparation N° - 33.10.04.99\n"
            "Dépose\n"
            "1. Déposer le premier ensemble\n"
            "situé dans le compartiment moteur.\n"
            "2. Dégager le boîtier de son support de fixation\n"
            "du module électronique.\n"
            "ATTENTION: Ne pas endommager les fils\n"
            "et les connecteurs pendant l'opération.\n"
            "MOTEUR\n10\nREPARATIONS\n"
            "3. Contrôler la fixation et conserver\n"
            "le couple indiqué à 9 N.m.\n"
            "4. Remonter la pièce.\n"
            "EMBRAYAGE");
        page.insert(QStringLiteral("content"),original);
        const QString answer=MemsDocumentaryRepository::answerFromHits(
            QVariantList{page});
        if(!require(answer.contains(QStringLiteral(
                            "2. Dégager le boîtier de son support de fixation "
                            "du module électronique.")) &&
                    answer.contains(QStringLiteral(
                            "1. Déposer le premier ensemble situé dans le "
                            "compartiment moteur.")),
                    "any printed PDF wrap must join inside its original step"))return 1;
        if(!require(answer.contains(QStringLiteral(
                           "ATTENTION: Ne pas endommager les fils "
                           "et les connecteurs pendant l'opération.")) &&
                    answer.contains(QStringLiteral(
                           "3. Contrôler la fixation et conserver "
                           "le couple indiqué à 9 N.m.")),
                    "precaution and torque values remain whole and unmodified"))return 1;
        if(!require(!answer.contains(QStringLiteral("MOTEUR\n")) &&
                    !answer.contains(QStringLiteral("\nREPARATIONS\n")) &&
                    !answer.contains(QStringLiteral("\nEMBRAYAGE\n")),
                    "mixed-order page heading and new chapter must not leak"))return 1;
        if(!require(answer.contains(QStringLiteral("\n2. ")) &&
                    answer.contains(QStringLiteral("\n3. ")) &&
                    answer.contains(QStringLiteral("\n4. ")) &&
                    !answer.contains(QStringLiteral("fixation\n"
                                                    "du module")),
                    "each numbered operation remains distinct and no visual wrap survives"))return 1;
        if(!require(page.value(QStringLiteral("content")).toString()==original &&
                    answer.contains(QStringLiteral("RCL0193FRE, page 180")),
                    "display reflow preserves raw manufacturer text and provenance"))return 1;
    }
    // Menu selection is one-based and always references the rows displayed,
    // including submenu entries under repeated headings.
    QVariantList genericMenu;
    for(int n=0;n<17;++n){
        QVariantMap row;
        row.insert(QStringLiteral("source_key"),QStringLiteral("doc:%1").arg(n));
        row.insert(QStringLiteral("source_page"),n+100);
        row.insert(QStringLiteral("title"),QStringLiteral("Composant %1").arg(n));
        genericMenu.append(row);
    }
    for(int n=1;n<=genericMenu.size();++n){
        const QVariantMap selected=MemsDocumentaryRepository::choiceForNumber(
            genericMenu,QString::number(n));
        if(!require(selected.value(QStringLiteral("source_key")).toString()
                    ==QStringLiteral("doc:%1").arg(n-1),
                    "menu number always resolves to its exact source row"))return 1;
    }
    for(const QString &invalid:{
        QStringLiteral("0"),QStringLiteral("-1"),QStringLiteral("18"),
        QStringLiteral("1x"),QStringLiteral("999999999999999999"),
        QStringLiteral(""),QStringLiteral("depose")}){
        if(!require(MemsDocumentaryRepository::choiceForNumber(
                        genericMenu,invalid).isEmpty(),
                    "invalid selection cannot open another document"))return 1;
    }
    const QVariantList repeated=MemsDocumentaryRepository::groupedChoices(
        QVariantList{genericMenu.at(0),genericMenu.at(0),
                     genericMenu.at(1)});
    if(!require(repeated.size()==2,"grouping keeps distinct entries"))return 1;
    const QVariantMap submenu=MemsDocumentaryRepository::choiceForNumber(
        repeated,QStringLiteral("1"));
    if(!require(submenu.value(QStringLiteral("iaMemsGroupRows"))
                    .toList().size()==2,
                "choosing a grouped heading opens its exact member list"))return 1;
    std::puts("MEMS_NATIVE_FRENCH_INTENT_AND_OPERATION_TEST_PASS");
    return 0;
}
