// Include after ux_review_workflow.inc to share its recorded mouse driver.
void SmokeTest::uxCompleteImageSave() {
    if (QGuiApplication::platformName() == "offscreen") QSKIP("Remote Windows/OpenGL desktop required");
    UxSettingsScope isolated;
    UxEvidence evidence("complete-image-save");
    QTemporaryDir output;
    QVERIFY(output.isValid());
    const QString path = output.filePath(QStringLiteral("structure.png"));
    MainWindow window;
    window.resize(1400, 900); window.show();
    const auto fixture = FileDispatcher::load("tests/data/mvp.out");
    QVERIFY(fixture.parseResult.success);
    window.onFileLoaded(fixture);
    QTest::qWait(150);
    QAction *action = uxNamed(window, "SaveImageDirect");
    if (!action) for (auto *candidate : window.findChildren<QAction *>())
        if (candidate->shortcuts().contains(QKeySequence("Ctrl+Q"))) { action = candidate; break; }
    QVERIFY(action);
    evidence.begin("save-transparent-png", "Loaded result; 1400x900 window; destination entered by keyboard",
                   "PNG written with transparent background, 512px long edge and 600 dpi metadata");
    QTimer driver;
    int dialogs = 0;
    bool failed = false;
    const auto editSize = [&](QSpinBox *spin) {
        if (!spin || !evidence.click(spin, "image long edge", QPoint(18, spin->height()/2))) return false;
        QTest::keyClick(spin, Qt::Key_A, Qt::ControlModifier);
        QTest::keyClicks(spin, "512");
        return true;
    };
    connect(&driver, &QTimer::timeout, &window, [&] {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog || dialog->property("uxSaveHandled").toBool()) return;
        dialog->setProperty("uxSaveHandled", true); ++dialogs;
        if (auto *file = qobject_cast<QFileDialog *>(dialog)) {
            if (auto *spin = file->findChild<QSpinBox *>("ImageSaveLongEdge")) {
                if (!editSize(spin)) { failed = true; file->reject(); return; }
                QVERIFY(file->findChild<QCheckBox *>("ImageSaveTransparent")->isChecked());
            }
            auto *name = file->findChild<QLineEdit *>("fileNameEdit");
            if (!name || !evidence.click(name, "destination filename")) { failed = true; file->reject(); return; }
            QTest::keyClick(name, Qt::Key_A, Qt::ControlModifier);
            QTest::keyClicks(name, QDir::toNativeSeparators(path));
            if (!evidence.capture(file, "image-save-options")) failed = true;
            auto *box = file->findChild<QDialogButtonBox *>();
            auto *save = box ? box->button(QDialogButtonBox::Save) : nullptr;
            QTimer::singleShot(0, file, [&, save] { if (!evidence.click(save, "save image")) failed = true; });
        } else if (auto *resolution = qobject_cast<QInputDialog *>(dialog)) {
            if (!editSize(resolution->findChild<QSpinBox *>())) { failed = true; resolution->reject(); return; }
            auto *box = resolution->findChild<QDialogButtonBox *>();
            auto *ok = box ? box->button(QDialogButtonBox::Ok) : nullptr;
            QTimer::singleShot(0, resolution, [&, ok] { if (!evidence.click(ok, "accept image size")) failed = true; });
        } else if (auto *question = qobject_cast<QMessageBox *>(dialog)) {
            if (question->windowTitle() != QStringLiteral("투명 배경")) {
                failed = true; question->reject(); return;
            }
            QTimer::singleShot(0, question, [&, question] {
                if (!evidence.click(question->button(QMessageBox::Yes), "transparent background")) failed = true;
            });
        } else { failed = true; dialog->reject(); }
    });
    driver.start(50);
    QTimer::singleShot(20000, &driver, [&] {
        driver.stop(); failed = true;
        if (auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget())) dialog->reject();
    });
    const bool route = uxAction(window, evidence, action);
    driver.stop();
    const QImage saved(path);
    const bool valid = route && !failed && !saved.isNull() && qMax(saved.width(), saved.height()) == 512
        && saved.hasAlphaChannel() && saved.pixelColor(0, 0).alpha() == 0 && saved.dotsPerMeterX() == 23622;
    evidence.end(valid);
    QVERIFY(evidence.write("workflow.json", QJsonObject{{"variant", evidence.variant}, {"dialogs", dialogs},
        {"clicks", evidence.clicks}, {"observed", valid}, {"width", saved.width()}, {"height", saved.height()},
        {"definition", "All left mouse clicks from shortest available save command through completed PNG; filename and size typed by keyboard. No setup or cleanup clicks."}}));
    QVERIFY2(valid, "Complete image save failed; inspect workflow.json, clicks.json and image-save-options.png");
}
