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

#include <k4aboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <klocalizedstring.h>

static constexpr const char description[] =
	I18N_NOOP("An IDE for microcontrollers and electronics");

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
	[[maybe_unused]] KAboutData about(
		QByteArray("ktechlab"),
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
	[[maybe_unused]] KApplication app;
	[[maybe_unused]] KTechlab *ktechlab = new KTechlab();
}
