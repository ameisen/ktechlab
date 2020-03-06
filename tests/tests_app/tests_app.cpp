/*
 * KTechLab: An IDE for microcontrollers and electronics
 * Copyright 2018  Zoltan Padrah <zoltan_padrah@users.sf.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "../src/ktechlab.h"
#include "config.h"
#include "docmanager.h"
#include "electronics/circuitdocument.h"

#include <k4aboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <klocalizedstring.h>

#include <QDebug>
#include <QTest>
#include <QTemporaryFile>

static constexpr const char description[] =
	I18N_NOOP("An IDE for microcontrollers and electronics");

class KtlTestsAppFixture final : public QObject {
	Q_OBJECT

public:
	KApplication *app = nullptr;
	KTechlab *ktechlab = nullptr;

private slots:
	void initTestCase() {
		[[maybe_unused]] int argc = 1;
		[[maybe_unused]] char argv0[] = "tests_app";
		[[maybe_unused]] char *argv[] = { argv0, nullptr };


		[[maybe_unused]] KAboutData about(
			"ktechlab",
			i18n("KTechLab"),
			VERSION,
			i18n(description),
			KAboutLicense::LicenseKey::GPL_V2,
			i18n("(C) 2003-2020, The KTechLab developers"),
			"",
			"https://userbase.kde.org/KTechlab",
			"ktechlab-devel@kde.org"
		);
		[[maybe_unused]] KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		app = new KApplication;
		ktechlab = new KTechlab;

	}
	void cleanupTestCase() {
		delete ktechlab;
		ktechlab = nullptr;
		// TODO : Isn't this the POINT of a test?
		//delete app; // this crashes apparently
		app = nullptr;
	}

	void testDocumentOpen() {
		DocManager::self()->closeAll();
		QCOMPARE( DocManager::self()->m_documentList.size(), 0);
		QFile exFile(SRC_TESTS_DATA_DIR "test-document-draw-1.circuit");
		KUrl exUrl(exFile.fileName());
		qDebug() << "open example: " << exUrl;
		DocManager::self()->openURL(exUrl, NULL);
		QCOMPARE( DocManager::self()->m_documentList.size(), 1);
		Document *doc = DocManager::self()->m_documentList.first();
		QVERIFY( doc != NULL );
		QCOMPARE( doc->type(), Document::dt_circuit );
		CircuitDocument *circDoc = static_cast<CircuitDocument*>( doc );
		QVERIFY( circDoc != NULL );
		QVERIFY( circDoc->m_canvas );
		qDebug() << "item list size " << circDoc->m_itemList.size();

		//QRect saveArea = circDoc->m_canvas->rect();   // is empty
		QRect resizeArea(0, -500, 400, 1080);
		qDebug() << " resizeArea " << resizeArea;
		circDoc->m_canvas->resize(resizeArea);

		QRect saveArea(-500, -500, 1040, 1080);
		qDebug() << "save area " << saveArea;
		QPixmap *outputImage = new QPixmap( saveArea.size() );
		outputImage->fill(Qt::green);

		circDoc->exportToImageDraw(saveArea, *outputImage);

		QImage img = dynamic_cast<QPixmap*>(outputImage)->toImage();
		img = img.copy();
		QTemporaryFile imgFile("testDocumentOpen_output_XXXXXX.png");
		imgFile.setAutoRemove(false);
		imgFile.open();
		qDebug() << "imgFile.fileName() = " << imgFile.fileName();
		bool saveResult = img.save(imgFile.fileName());
		QCOMPARE( saveResult, true );

		delete outputImage;

		DocManager::self()->closeAll();
		QCOMPARE( DocManager::self()->m_documentList.size(), 0);
	}
};

QTEST_MAIN(KtlTestsAppFixture)
#include "tests_app.moc"
