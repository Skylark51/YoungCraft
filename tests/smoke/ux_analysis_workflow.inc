void SmokeTest::uxAnalysisClickMeasurements() {
    if (QGuiApplication::platformName() == "offscreen") QSKIP("Remote Windows/OpenGL desktop required");
    UxSettingsScope isolated;
    UxEvidence evidence("analysis-clicks");
    auto fixture = FileDispatcher::load("tests/data/mvp.out");
    QVERIFY(fixture.parseResult.success);
    auto &data = fixture.parseResult.data;
    data.frames.clear(); data.scanPoints.clear();
    for (int i = 0; i < 3; ++i) {
        Frame frame;
        frame.metrics.energyHartree = -10 + .01 * i;
        frame.metrics.step = i;
        for (const auto &atom : data.molecule.atoms) frame.coordinates.append(atom.position + QVector3D(.1f*i, 0, 0));
        data.frames.append(frame);
        ScanPoint point;
        point.index = i + 1; point.coordinate = 1 + i; point.relativeKcal = i;
        point.energyHartree = frame.metrics.energyHartree; point.frameIndex = i;
        data.scanPoints.append(point);
    }
    for (bool scan : {false, true}) {
        MainWindow window;
        window.resize(1400, 900); window.show(); window.onFileLoaded(fixture);
        window.m_analysis->setCurrentTab(scan ? 2 : 0); window.m_animation->stop();
        QTest::qWait(150);
        evidence.begin(scan ? "scan-graph-select" : "optimization-graph-select",
                       "Corresponding analysis panel active; separate graph window closed",
                       "Second graph point selected and matching molecular coordinates applied");
        auto *plot = window.m_analysis->findChild<PlotWidget *>(scan ? "ScanEnergyPreview" : "OptimizationEnergyPreview");
        if (!plot) {
            auto *open = window.m_analysis->findChild<QPushButton *>(scan ? "OpenScanGraph" : "OpenOptimizationGraph");
            QVERIFY(evidence.click(open, "open full graph"));
            plot = window.m_analysis->findChild<PlotWidget *>(scan ? "ScanResultsPlot" : "OptimizationEnergyPlot");
            QTest::qWait(100);
        }
        QVERIFY(plot); QVERIFY(plot->isVisible()); QVERIFY(plot->selectablePoints().size() > 1);
        const auto point = plot->selectablePoints()[1];
        const auto range = plot->axisRange(); const auto area = plot->plotArea();
        const QPoint pixel(qRound(area.left() + (point.x()-range.xMin)/(range.xMax-range.xMin)*area.width()),
                           qRound(area.bottom() - (point.y()-range.yMin)/(range.yMax-range.yMin)*area.height()));
        QVERIFY(evidence.click(plot, "second graph point", pixel));
        QCOMPARE(plot->selectedPoint(), 1);
        QCOMPARE(window.m_currentFrame, 1);
        QCOMPARE(window.m_view->molecule().atoms[0].position, data.frames[1].coordinates[0]);
        QVERIFY(evidence.capture(plot, scan ? "scan-graph" : "optimization-graph"));
        evidence.end(true);
    }
    {
        StyleEditorDialog style(StyleManager::preset(QStringLiteral("Youngcraft Studio")));
        style.resize(720, 520); style.show(); QTest::qWait(100);
        evidence.begin("style-atom-size", "Style editor just opened at its default page",
                       "Atom size changed and live style update emitted");
        auto *slider = style.findChild<QSlider *>("AtomScaleSlider");
        QVERIFY(slider);
        if (!slider->isVisible()) {
            auto *tabs = style.findChild<QTabWidget *>(); QVERIFY(tabs);
            int atomTab = -1;
            for (int i=0; i<tabs->count(); ++i) if (tabs->tabText(i).startsWith(QStringLiteral("원자"))) { atomTab=i; break; }
            QVERIFY(atomTab >= 0);
            QVERIFY(evidence.click(tabs->tabBar(), "atom settings", tabs->tabBar()->tabRect(atomTab).center()));
        }
        QSignalSpy changed(&style, &StyleEditorDialog::styleApplied);
        const auto previous = slider->value();
        QVERIFY(evidence.click(slider, "atom size", QPoint(slider->width()*3/4, slider->height()/2)));
        QVERIFY(slider->value() != previous); QVERIFY(!changed.isEmpty());
        QVERIFY(evidence.capture(&style, "atom-size-style"));
        evidence.end(true); style.reject();
    }
    {
        StyleEditorDialog style(StyleManager::preset(QStringLiteral("Youngcraft Studio")));
        style.resize(720, 520); style.show(); QTest::qWait(100);
        auto *tabs = style.findChild<QTabWidget *>();
        auto *preset = style.findChild<QComboBox *>("StylePreset");
        QVERIFY(tabs); QVERIFY(preset);
        int atoms = -1, presets = -1;
        for (int i=0; i<tabs->count(); ++i) {
            if (tabs->tabText(i).startsWith(QStringLiteral("원자"))) atoms=i;
            if (tabs->widget(i)->isAncestorOf(preset)) presets=i;
        }
        QVERIFY(atoms >= 0);
        tabs->setCurrentIndex(atoms); QTest::qWait(50);
        evidence.begin("style-preset-return", "Atom settings active in an open style editor",
                       "Dark Studio preset applied with atom settings available again");
        if (!preset->isVisible()) {
            QVERIFY(presets >= 0);
            QVERIFY(evidence.click(tabs->tabBar(), "preset settings", tabs->tabBar()->tabRect(presets).center()));
        }
        const int row = preset->findText(QStringLiteral("Dark Studio")); QVERIFY(row >= 0);
        QVERIFY(evidence.click(preset, "choose preset"));
        auto *choices = preset->view();
        const auto item = preset->model()->index(row, 0);
        choices->scrollTo(item);
        QTest::qWait(50);
        const auto choicePosition = choices->visualRect(item).center();
        QTest::mouseMove(choices->viewport(), choicePosition);
        QTest::qWait(50);
        QVERIFY(evidence.click(choices->viewport(), "Dark Studio", choicePosition));
        QTest::qWait(50);
        qInfo() << "Preset mouse selection:" << preset->currentIndex() << preset->currentText();
        QVERIFY(evidence.capture(&style, "preset-after-selection"));
        if (tabs->currentIndex() != atoms)
            QVERIFY(evidence.click(tabs->tabBar(), "return to atom settings", tabs->tabBar()->tabRect(atoms).center()));
        QCOMPARE(style.style().name, QStringLiteral("Dark Studio"));
        QCOMPARE(tabs->currentIndex(), atoms);
        QVERIFY(evidence.capture(&style, "preset-in-atom-settings"));
        evidence.end(true); style.reject();
    }
}
