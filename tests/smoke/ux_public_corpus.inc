// Real parser outputs exercise the same widgets as the synthetic interaction tests.
void SmokeTest::uxPublicCalculationFiles() {
    const QDir corpus(qEnvironmentVariable("YOUNGCRAFT_NOVEMBER_CORPUS"));
    if (qEnvironmentVariableIsEmpty("YOUNGCRAFT_NOVEMBER_CORPUS"))
        QSKIP("Set YOUNGCRAFT_NOVEMBER_CORPUS to the checksum-verified public corpus");
    struct Case { const char *path; int tab; };
    const Case cases[] = {
        {"Gaussian/Gaussian03/dvb_gopt.out", 0},
        {"Gaussian/Gaussian03/dvb_ir.out", 1},
        {"Gaussian/Gaussian03/dvb_td.out", 4},
        {"Gaussian/Gaussian09/dvb_scan_revA.02.log", 2},
        {"Gaussian/Gaussian09/ansaBPFP2_acetylene_ircf_LQA.out", 3},
        {"ORCA/ORCA4.0/H2-_CASSCF_opt.out", 0},
        {"ORCA/ORCA4.0/dvb_ir.out", 1},
        {"ORCA/ORCA4.0/dvb_td.out", 4},
        {"ORCA/ORCA4.2/dvb_scan.out", 2},
        {"ORCA/ORCA6.1/1509.out", 4}
    };
    for (const auto &sample : cases) {
        const QString path = corpus.filePath(QString::fromLatin1(sample.path));
        QVERIFY2(QFileInfo::exists(path), qPrintable(path));
        MainWindow window;
        window.resize(1400, 900);
        window.show();
        window.openFile(path);
        auto *page = window.m_documentTabs->currentWidget();
        QVERIFY(page);
        QTRY_VERIFY_WITH_TIMEOUT(page->property("loadFinished").toBool(), 30000);
        QVERIFY2(page->property("loadSuccess").toBool(), sample.path);
        QVERIFY2(window.m_analysis->hasTabData(sample.tab), sample.path);
        auto *action = window.findChild<QAction *>(QStringLiteral("AnalysisPanelTab%1").arg(sample.tab));
        auto *button = toolButtonForAction(window.m_explorerDock, action);
        QVERIFY(action); QVERIFY(button); QVERIFY(button->isVisible());
        QTest::mouseClick(button, Qt::LeftButton);
        QCOMPARE(window.m_analysis->currentTab(), sample.tab);
        const auto verifyFrame = [&](int frame) {
            if (frame < 0 || frame >= window.m_data.frames.size()) return false;
            const auto &coordinates = window.m_data.frames[frame].coordinates;
            if (coordinates.size() != window.m_view->molecule().atoms.size()) return false;
            for (int i = 0; i < coordinates.size(); ++i)
                if ((coordinates[i] - window.m_view->molecule().atoms[i].position).length() > 1.e-5f) return false;
            return true;
        };
        const auto clickFirst = [](QListWidget *list) {
            if (!list || !list->count()) return false;
            list->scrollToItem(list->item(0));
            QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::NoModifier,
                              list->visualItemRect(list->item(0)).center());
            return list->currentRow() == 0;
        };
        if (sample.tab == 0) {
            QVERIFY(clickFirst(window.m_analysis->findChild<QListWidget *>("FrameList")));
            QVERIFY2(verifyFrame(0), sample.path);
        } else if (sample.tab == 1) {
            QVERIFY(clickFirst(window.m_analysis->findChild<QListWidget *>("FrequencyList")));
            QCOMPARE(window.m_currentFrequency, 0);
            bool hasMotion = false;
            for (const auto &vector : window.m_data.frequencies[0].displacement)
                hasMotion = hasMotion || vector.lengthSquared() > 1.e-12f;
            if (hasMotion) QVERIFY(window.m_animation->isPlaying());
            window.m_animation->stop();
        } else if (sample.tab == 2) {
            QVERIFY(clickFirst(window.m_analysis->findChild<QListWidget *>("ScanList")));
            const int frame = window.m_data.scanPoints[0].frameIndex;
            if (frame >= 0) QVERIFY2(verifyFrame(frame), sample.path);
            if (auto *preview = window.m_analysis->findChild<PlotWidget *>("ScanEnergyPreview"))
                QCOMPARE(preview->selectedPoint(), 0);
        } else if (sample.tab == 3) {
            QVERIFY(clickFirst(window.m_analysis->findChild<QListWidget *>("IrcPointList")));
            const int frame = window.m_data.ircPoints[0].frameIndex;
            QVERIFY2(verifyFrame(frame), sample.path);
            QCOMPARE(window.m_analysis->findChild<PlotWidget *>("IrcEnergyPlot")->selectedPoint(), 0);
        } else {
            auto *states = window.m_analysis->findChild<QTableWidget *>("TddftStatesTable");
            QVERIFY(states); QVERIFY(states->rowCount() > 0);
            states->scrollToItem(states->item(0, 0));
            QTest::mouseClick(states->viewport(), Qt::LeftButton, Qt::NoModifier,
                              states->visualItemRect(states->item(0, 0)).center());
            QCOMPARE(states->currentRow(), 0);
            QCOMPARE(window.m_tddftWorkspacePlot->selectedPoint(), 0);
            const int frame = window.m_data.excitedStates[0].frameIndex;
            if (frame >= 0) QVERIFY2(verifyFrame(frame), sample.path);
        }
        qInfo().noquote() << "Actual file GUI selection verified:" << sample.path;
        window.close();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
}
