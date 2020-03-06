/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef GPLIB_H
#define GPLIB_H

#include <externallanguage.h>

/**
@author David Saxton
*/
class Gplib : public ExternalLanguage
{
	public:
		Gplib( ProcessChain *processChain );
		~Gplib() override;

		void processInput(const ProcessOptions &options) override;
		MessageInfo extractMessageInfo( const QString &text ) override;
		ProcessOptions::Path outputPath( ProcessOptions::Path inputPath ) const override;

	protected:
		bool isError( const QString &message ) const override;
		bool isWarning( const QString &message ) const override;
};

#endif
