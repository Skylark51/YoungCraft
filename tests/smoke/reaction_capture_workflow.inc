// Included by test_smoke.cpp so this workflow has the same private test access.
#include <QBuffer>
#include "ui/MultiCaptureDialog.h"
void SmokeTest::multiCaptureWorkflow() {
    if(QGuiApplication::platformName()=="offscreen")QSKIP("Requires OpenGL");
    MainWindow first,second;first.show();second.show();
    first.openFile("tests/data/mvp.out");second.openFile("tests/data/mvp.out");
    QTRY_VERIFY_WITH_TIMEOUT(!first.m_data.molecule.isEmpty() && !second.m_data.molecule.isEmpty(),15000);
    Molecule single;single.atoms={{1,6,"C",{0,0,0}}};second.m_view->setMolecule(single);
    first.openMultiCapture();
    auto *dialog=dynamic_cast<MultiCaptureDialog *>(first.findChild<QDialog *>("MultiCaptureDialog"));QVERIFY(dialog);
    QCOMPARE(dialog->m_views.size(),2);QTest::qWait(200);
    QVERIFY(dialog->m_views[0]->molecule().atoms.size()!=dialog->m_views[1]->molecule().atoms.size());
    const auto original=first.m_view->cameraState();
    auto *view=dialog->m_views[0];auto camera=view->cameraState();camera.rotation={};camera.perspective=false;view->setCameraState(camera);
    auto *axis=dialog->findChild<QComboBox *>("CaptureRotationAxis");QVERIFY(axis);
    for(int index:{1,2}) {
        axis->setCurrentIndex(index);camera.rotation={};view->setCameraState(camera);view->rotateBy({20,10});
        const auto q=view->cameraState().rotation;QVERIFY(qAbs(index==1?q.x():q.y())>.001f);QVERIFY(qAbs(q.z())<.0001f);
    }
    axis->setCurrentIndex(0);camera.rotation={};view->setCameraState(camera);view->rotateBy({20,10});
    const auto free=view->cameraState().rotation;QVERIFY(qAbs(free.x())>.001f && qAbs(free.y())>.001f);QVERIFY(qAbs(free.length()-1)<.0001f);
    view->panBy({7,9});QVERIFY((dialog->m_views[1]->cameraState().centerOffset-view->cameraState().centerOffset).length()<1e-5f);
    QWheelEvent wheel(QPointF(20,20),view->mapToGlobal(QPoint(20,20)),{},{0,120},Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
    const auto oldScale=view->cameraState().orthoScale;QApplication::sendEvent(view,&wheel);QVERIFY(view->cameraState().orthoScale!=oldScale);
    QCOMPARE(dialog->m_views[1]->cameraState().orthoScale,view->cameraState().orthoScale);
    axis->setCurrentIndex(3);camera.rotation={};view->setCameraState(camera);
    view->rotateBy({20,10});
    const auto rotated=view->cameraState();QVERIFY(qAbs(rotated.rotation.z())>.001f);QVERIFY(qAbs(rotated.rotation.x())<.0001f);QVERIFY(qAbs(rotated.rotation.y())<.0001f);
    QCOMPARE(dialog->m_views[1]->cameraState().rotation,rotated.rotation);
    QCOMPARE(first.m_view->cameraState().rotation,original.rotation);
    const auto preview=qEnvironmentVariable("YOUNGCRAFT_MULTI_CAPTURE_PREVIEW");if(!preview.isEmpty())QVERIFY(dialog->grab().save(preview));
    const auto panels=dialog->renderPanels();QCOMPARE(panels.size(),2);QVERIFY(!panels[0].isNull());QCOMPARE(panels[0].size(),panels[1].size());QCOMPARE(panels[0].pixelColor(0,0).alpha(),0);
    dialog->m_list->item(1)->setCheckState(Qt::Unchecked);view->rotateBy({10,0});QCOMPARE(dialog->m_views[1]->cameraState().rotation,rotated.rotation);
    QCOMPARE(dialog->renderPanels().size(),1);
    dialog->exportPanels(false);QCOMPARE(QApplication::clipboard()->mimeData()->html().count("<img "),1);
    QTemporaryDir saved;QVERIFY(saved.isValid());dialog->exportPanels(true,saved.path());
    const auto folders=QDir(saved.path()).entryList({"Youngcraft-Multi-*"},QDir::Dirs|QDir::NoDotAndDotDot);QCOMPARE(folders.size(),1);
    QCOMPARE(QDir(saved.path()+"/"+folders.first()).entryList({"*.png"},QDir::Files).size(),1);
    second.closeDocument(0);QCOMPARE(dialog->renderPanels().size(),1);
    dialog->findChild<QPushButton *>("CaptureResetView")->click();QCOMPARE(view->cameraState().rotation,dialog->m_sources[0].camera.rotation);
    dialog->m_list->item(0)->setCheckState(Qt::Unchecked);QVERIFY(dialog->renderPanels().isEmpty());
    dialog->close();QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
void SmokeTest::reactionCaptureHydrogenControls() {
    if(QGuiApplication::platformName()=="offscreen")QSKIP("Requires OpenGL");
    QTemporaryDir directory;QVERIFY(directory.isValid());
    CalculationData data;data.filePath=directory.filePath("hydrogens-freq-ts-freq.out");
    data.molecule.atoms={{1,6,"C",{0,0,0}},{2,1,"H",{1.09f,0,0}},{3,8,"O",{0,2,0}},{4,1,"H",{0,2.96f,0}}};
    data.molecule.rebuildBonds();
    FrequencyMode mode;mode.imaginary=true;mode.wavenumber=-120;mode.displacement={{0,0,0},{0.05f,0,0},{0,0,0},{0,0.02f,0}};data.frequencies={mode};
    const QByteArray output=
        " Standard orientation:\n ---------------------------------------------------------------------\n"
        " Center Atomic Atomic Coordinates (Angstroms)\n Number Number Type X Y Z\n"
        " ---------------------------------------------------------------------\n"
        " 1 6 0 0.0 0.0 0.0\n 2 1 0 1.09 0.0 0.0\n 3 8 0 0.0 2.0 0.0\n 4 1 0 0.0 2.96 0.0\n"
        " ---------------------------------------------------------------------\n Normal termination of Gaussian 16\n";
    for(const auto &path:ReactionCapture::companionPaths(data.filePath)){QFile f(path);QVERIFY(f.open(QIODevice::WriteOnly));f.write(output);}
    MoleculeView source;source.resize(500,500);source.setMolecule(data.molecule);source.show();
    RenderStyle style;style.perspective=false;style.showHydrogen=true;source.setStyle(style);
    auto *dialog=new ReactionCaptureDialog(data,&source,0.5,0.2,true,&source,false);dialog->show();
    QTRY_VERIFY_WITH_TIMEOUT(dialog->m_ready,10000);QTest::qWait(100);
    const auto sourceMolecule=source.molecule();
    auto camera=dialog->m_camera;camera.orthoScale=3.0f;camera.centerOffset={};camera.rotation={};dialog->synchronize(camera);
    auto *hide=dialog->findChild<QPushButton *>("HideCaptureCarbonHydrogens");
    auto *keep=dialog->findChild<QPushButton *>("HideCaptureUnselectedHydrogens");
    auto *restore=dialog->findChild<QPushButton *>("RestoreCaptureHydrogens");
    QVERIFY(hide && keep && restore);QCOMPARE(dialog->m_hydrogens->count(),2);
    keep->click();QCOMPARE(dialog->m_hydrogenVisibility,ReactionCaptureDialog::HydrogenVisibility::Original);
    hide->click();for(auto *view:dialog->m_views){QVERIFY(!view->molecule().atoms[1].visible);QVERIFY(view->molecule().atoms[3].visible);}
    restore->click();for(auto *view:dialog->m_views){QVERIFY(view->molecule().atoms[1].visible);QVERIFY(view->molecule().atoms[3].visible);}
    auto *ts=dialog->m_views[1];
    const auto point=ts->project(ts->molecule().atoms[1].position,ts->viewProjection());
    QTest::mouseClick(ts,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(point.x()),qRound(point.y())));
    QVERIFY(dialog->m_keptHydrogens.contains(1));QCOMPARE(dialog->m_hydrogens->item(0)->checkState(),Qt::Checked);
    keep->click();
    for(auto *view:dialog->m_views){QVERIFY(view->molecule().atoms[1].visible);QVERIFY(!view->molecule().atoms[3].visible);}
    dialog->m_hydrogens->item(1)->setCheckState(Qt::Checked);
    dialog->m_hydrogens->item(0)->setCheckState(Qt::Unchecked);
    for(auto *view:dialog->m_views){QVERIFY(!view->molecule().atoms[1].visible);QVERIFY(view->molecule().atoms[3].visible);}
    const auto panels=dialog->renderPanels();QCOMPARE(panels.size(),6);QVERIFY(dialog->m_animation->isPlaying());
    dialog->findChild<QPushButton *>("SwapReactionSides")->click();
    for(auto *view:dialog->m_views){QVERIFY(!view->molecule().atoms[1].visible);QVERIFY(view->molecule().atoms[3].visible);}
    const auto withoutCarbonH=panels[1];
    dialog->m_hydrogens->item(0)->setCheckState(Qt::Checked);
    const auto withCarbonH=dialog->renderPanels()[1];QVERIFY(withoutCarbonH!=withCarbonH);
    restore->click();
    for(auto *view:dialog->m_views){QVERIFY(view->molecule().atoms[1].visible);QVERIFY(view->molecule().atoms[3].visible);QCOMPARE(view->cameraState().rotation,camera.rotation);}
    // Capture-specific controls must also override an inherited global H-off style.
    for(auto *view:dialog->m_views){auto s=view->style();s.showHydrogen=false;view->setStyle(s);}
    keep->click();for(auto *view:dialog->m_views)QVERIFY(view->style().showHydrogen);
    for(int i=0;i<sourceMolecule.atoms.size();++i){QCOMPARE(source.molecule().atoms[i].position,sourceMolecule.atoms[i].position);QCOMPARE(source.molecule().atoms[i].visible,sourceMolecule.atoms[i].visible);}
    QCOMPARE(source.molecule().bonds.size(),sourceMolecule.bonds.size());
    // SI always captures the optimized, undisplaced TS as a static transparent PNG.
    dialog->m_si->setChecked(true);QVERIFY(!dialog->m_animation->isPlaying());
    for(int i=0;i<data.molecule.atoms.size();++i)QCOMPARE(dialog->m_views[1]->molecule().atoms[i].position,data.molecule.atoms[i].position);
    dialog->m_structureName->setText("water reaction");dialog->m_figureNumber->setText("S12");
    QVERIFY(dialog->m_caption->toPlainText().startsWith("Figure S12. Optimized structures of water reaction."));
    QVERIFY(dialog->m_caption->toPlainText().contains("atoms, respectively."));
    const auto still=dialog->renderPanels();QCOMPARE(still.size(),3);QVERIFY(!dialog->m_animation->isPlaying());
    for(const auto &image:still){QCOMPARE(image.pixelColor(0,0).alpha(),0);QCOMPARE(image.size(),still[0].size());}
    dialog->exportPanels(0);auto *si=QApplication::clipboard()->mimeData();
    QVERIFY(!si->hasFormat("image/gif"));QCOMPARE(si->html().count("<img "),3);QVERIFY(si->text().contains("Figure S12."));
    dialog->m_includeTs->setChecked(false);dialog->exportPanels(0);si=QApplication::clipboard()->mimeData();
    QCOMPARE(si->html().count("<img "),2);QVERIFY(!si->hasFormat("image/gif"));
    dialog->exportStaticPanels(4,directory.path());
    const auto folders=QDir(directory.path()).entryList({"Youngcraft-SI-*"},QDir::Dirs|QDir::NoDotAndDotDot);QCOMPARE(folders.size(),1);
    const QDir saved(directory.path()+"/"+folders.first());QCOMPARE(saved.entryList({"*.png"},QDir::Files).size(),2);QCOMPARE(saved.entryList({"*.gif"},QDir::Files).size(),0);
    QVERIFY(saved.exists("caption.txt"));QVERIFY(saved.exists("paste.html"));
    dialog->m_includeTs->setChecked(true);dialog->exportStaticPanels(4,directory.path());
    const auto preview=qEnvironmentVariable("YOUNGCRAFT_SI_PREVIEW");
    if(!preview.isEmpty()){QTest::qWait(100);QVERIFY(dialog->grab().save(preview));}
    const auto capture=qEnvironmentVariable("YOUNGCRAFT_REACTION_CAPTURE_DIR");
    if(!capture.isEmpty())QVERIFY(dialog->grab().save(capture+"/hydrogen-controls.png"));
    dialog->close();QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}

void SmokeTest::reactionCaptureWorkflow() {
    if(QGuiApplication::platformName()=="offscreen")QSKIP("Requires OpenGL");
    QTemporaryDir directory;QVERIFY(directory.isValid());
    const auto tsPath=directory.filePath(QStringLiteral("한글-freq-ts-freq.out"));
    QVERIFY(QFile::copy("tests/data/mvp.out",tsPath));
    for(const auto &path:ReactionCapture::companionPaths(tsPath))QVERIFY(QFile::copy("tests/data/mvp.out",path));
    MainWindow window;window.show();window.openFile(tsPath);
    QTRY_COMPARE_WITH_TIMEOUT(window.m_currentFile,tsPath,15000);
    QTest::qWait(150);
    QVERIFY(!window.findChild<QDialog *>("ReactionCaptureDialog"));
    auto *exportAction = window.findChild<QAction *>("ToolbarExport");
    QVERIFY(exportAction && exportAction->menu());
    auto *exportMenu = exportAction->menu();
    QVERIFY(QMetaObject::invokeMethod(exportMenu, "aboutToShow"));
    QAction *captureAction = nullptr;
    for (auto *action : exportMenu->actions()) {
        if (action->text() == QStringLiteral("IRC·TS 캡처…")) captureAction = action;
    }
    QVERIFY(captureAction && captureAction->isEnabled());
    captureAction->trigger();
    QTRY_VERIFY_WITH_TIMEOUT(window.findChild<QDialog *>("ReactionCaptureDialog"),15000);
    auto *dialog=dynamic_cast<ReactionCaptureDialog *>(window.findChild<QDialog *>("ReactionCaptureDialog"));QVERIFY(dialog);
    QVERIFY(dialog->m_si->isChecked());QVERIFY(!dialog->m_animation->isPlaying());
    dialog->m_si->setChecked(false);
    dialog->findChild<QComboBox *>("CaptureOfficePreset")->setCurrentIndex(0);
    QTRY_VERIFY_WITH_TIMEOUT(dialog->m_ready,15000);
    QTest::qWait(150);QVERIFY(dialog->m_views[1]->isValid());
    const auto source=window.m_view->molecule();
    window.openReactionCapture();QCOMPARE(window.findChildren<QDialog *>("ReactionCaptureDialog").size(),1);
    auto camera=dialog->m_views[1]->cameraState();camera.rotation=QQuaternion::fromEulerAngles(22,35,14);camera.centerOffset={};camera.orthoScale=1.8f;camera.distance=8;
    dialog->synchronize(camera);
    QCOMPARE(dialog->m_views[0]->cameraState().rotation,camera.rotation);
    QCOMPARE(dialog->m_views[2]->cameraState().orthoScale,camera.orthoScale);
    const auto oldLeft=dialog->m_names[0]->text();dialog->findChild<QPushButton *>("SwapReactionSides")->click();
    QCOMPARE(dialog->m_names[2]->text(),oldLeft);
    dialog->m_width->setValue(5);dialog->m_cycle=0.2;dialog->exportPanels(0);
    const auto *mime=QApplication::clipboard()->mimeData();QVERIFY(mime->hasHtml());QVERIFY(mime->hasFormat("image/gif"));
    QCOMPARE(mime->html().count("<img "),3);QVERIFY(mime->html().contains("width:5.0000cm"));
    QByteArray gif=mime->data("image/gif");QBuffer buffer(&gif);buffer.open(QIODevice::ReadOnly);QImageReader reader(&buffer,"gif");
    QCOMPARE(reader.imageCount(),4);const auto row=reader.read();QVERIFY(!row.isNull());QCOMPARE(row.width(),3000);QCOMPARE(row.pixelColor(0,0).alpha(),0);
    const auto probe=qEnvironmentVariable("YOUNGCRAFT_REACTION_OFFICE_PROBE");
    if(!probe.isEmpty()) {
        QProcess process;process.start("powershell.exe",{"-NoProfile","-ExecutionPolicy","Bypass","-File",probe});
        QTRY_VERIFY_WITH_TIMEOUT(process.state()==QProcess::NotRunning,60000);
        qInfo().noquote()<<process.readAllStandardOutput();QVERIFY2(process.exitCode()==0,process.readAllStandardError().constData());
        QImageReader embedded(qEnvironmentVariable("YOUNGCRAFT_REACTION_CAPTURE_DIR")+"/image2.gif");
        QCOMPARE(embedded.imageCount(),4);
        QCOMPARE(embedded.read().pixelColor(0,0).alpha(),0);
    }
    const auto capture=qEnvironmentVariable("YOUNGCRAFT_REACTION_CAPTURE_DIR");
    if(!capture.isEmpty()) {
        QDir().mkpath(capture);QTest::qWait(100);QVERIFY(dialog->grab().save(capture+"/preview.png"));
        QFile out(capture+"/row.gif");QVERIFY(out.open(QIODevice::WriteOnly));out.write(gif);
    }
    dialog->exportPanels(1);
    const auto png=QImage::fromData(QApplication::clipboard()->mimeData()->data("image/png"));
    QCOMPARE(png.width(),1000);QCOMPARE(png.dotsPerMeterX(),20000);QCOMPARE(png.pixelColor(0,0).alpha(),0);
    dialog->findChild<QComboBox *>("CaptureOfficePreset")->setCurrentIndex(1);
    dialog->m_width->setValue(10);dialog->exportPanels(3);
    const auto ppt=QImage::fromData(QApplication::clipboard()->mimeData()->data("image/png"));QCOMPARE(ppt.dotsPerMeterX(),10000);QCOMPARE(ppt.size(),png.size());
    if(!probe.isEmpty()) {
        dialog->exportPanels(0);
        QProcess process;auto env=QProcessEnvironment::systemEnvironment();env.insert("YOUNGCRAFT_REACTION_PROBE_TARGET","PowerPoint");process.setProcessEnvironment(env);
        process.start("powershell.exe",{"-NoProfile","-ExecutionPolicy","Bypass","-File",probe});
        QTRY_VERIFY_WITH_TIMEOUT(process.state()==QProcess::NotRunning,60000);
        qInfo().noquote()<<process.readAllStandardOutput();QVERIFY2(process.exitCode()==0,process.readAllStandardError().constData());
        QImageReader embedded(qEnvironmentVariable("YOUNGCRAFT_REACTION_CAPTURE_DIR")+"/ppt-embedded-TS.gif");
        QCOMPARE(embedded.imageCount(),4);QCOMPARE(embedded.read().pixelColor(0,0).alpha(),0);
    }
    dialog->exportPanels(2);QCOMPARE(QApplication::clipboard()->mimeData()->html().count("<img "),1);
    QVERIFY(QApplication::clipboard()->mimeData()->html().contains("width:10.0000cm"));
    for(int i=0;i<source.atoms.size();++i)QCOMPARE(window.m_view->molecule().atoms[i].position,source.atoms[i].position);
    QVERIFY(dialog->m_animation->isPlaying());
    dialog->close();QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
    auto data=FileDispatcher::load(tsPath).parseResult.data;
    QFile::remove(ReactionCapture::companionPaths(tsPath)[1]);
    auto *missing=new ReactionCaptureDialog(data,window.m_view,0.5,0.2,false,&window);missing->show();
    QTRY_VERIFY_WITH_TIMEOUT(missing->m_status->text().contains(QStringLiteral("읽을 수 없습니다")),15000);
    QVERIFY(!missing->m_ready);for(auto *button:missing->m_exports)QVERIFY(!button->isEnabled());missing->close();
}
