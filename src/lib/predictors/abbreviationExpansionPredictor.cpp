
/******************************************************
 *  Presage, an extensible predictive text entry system
 *  ---------------------------------------------------
 *
 *  Copyright (C) 2008  Matteo Vescovi <matteo.vescovi@yahoo.co.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
                                                                             *
									     **********(*)*/


#include "abbreviationExpansionPredictor.h"

#include <fstream>


AbbreviationExpansionPredictor::AbbreviationExpansionPredictor(Configuration* config, ContextTracker* ct, const char* name)
    : Predictor(config,
		ct,
		name,
		"AbbreviationExpansionPredictor, maps abbreviations to the corresponding fully expanded token.",
		"AbbreviationExpansionPredictor maps abbreviations to the corresponding fully expanded token (i.e. word or phrase).\n\nThe mapping between abbreviations and expansions is stored in the file specified by the predictor configuration section.\n\nThe format for the abbreviation-expansion database is a simple tab separated text file format, with each abbreviation-expansion pair per line."
	),
      dispatcher (this),
      prepend_backspaces(true)
{
    LOGGER        = PREDICTORS + name + ".LOGGER";
    ABBREVIATIONS = PREDICTORS + name + ".ABBREVIATIONS";
    PREPEND_BACKSPACES = PREDICTORS + name + ".PREPEND_BACKSPACES";

    // build notification dispatch map
    dispatcher.map (config->find (LOGGER), & AbbreviationExpansionPredictor::set_logger);
    dispatcher.map (config->find (ABBREVIATIONS), & AbbreviationExpansionPredictor::set_abbreviations);
    dispatcher.map (config->find (PREPEND_BACKSPACES), & AbbreviationExpansionPredictor::set_prepend_backspaces);
}

AbbreviationExpansionPredictor::~AbbreviationExpansionPredictor()
{
    // complete
}


void AbbreviationExpansionPredictor::set_abbreviations (const std::string& filename)
{
    abbreviations = filename;
    logger << INFO << "ABBREVIATIONS: " << abbreviations << endl;

    cacheAbbreviationsExpansions();
}

void AbbreviationExpansionPredictor::set_prepend_backspaces (const std::string& value)
{
    prepend_backspaces = Utility::isYes(value);
    logger << INFO << "PREPEND_BACKSPACES: " << PREPEND_BACKSPACES << endl;
}


Prediction AbbreviationExpansionPredictor::predict(const size_t max_partial_predictions_size, const char** filter) const
{
    Prediction result;

    std::string prefix = contextTracker->getPrefix();

    std::map< std::string, std::string >::const_iterator it = cache.find(prefix);

    if (it != cache.end()) {
        std::string prediction(it->second);
        if (prepend_backspaces) {
            // prepend expansion with enough backspaces to erase
            // abbreviation
            std::string expansion(prefix.size(), '\b');

            // concatenate actual expansion
            prediction = expansion + prediction;
        }
        result.addSuggestion(Suggestion(prediction, 1.0));

    } else {
        logger << NOTICE << "Could not find expansion for abbreviation: " << prefix << endl;
    }

    return result;
}

void AbbreviationExpansionPredictor::learn (const std::vector<std::string>& change)
{
    // intentionally empty
}

void AbbreviationExpansionPredictor::forget (const std::string& word)
{
    // intentionally empty
}

void AbbreviationExpansionPredictor::cacheAbbreviationsExpansions()
{
    cache.clear();

    std::ifstream abbr_file(abbreviations.c_str());
    if (!abbr_file) {
        logger << ERROR << "Could not open abbreviations file: " << abbreviations << endl;
        // TODO: throw exception here
        //

    } else {
        logger << INFO << "Caching abbreviations/expansions from file: " << abbreviations << endl;
    
        std::string buffer;
        std::string abbreviation;
        std::string expansion;
        std::string::size_type tab_pos;
        while (getline(abbr_file, buffer)) {
            tab_pos = buffer.find_first_of('\t');
            if (tab_pos == std::string::npos) {
                logger << ERROR << "Error reading abbreviations/expansions from file: " << abbreviations << endl;
            } else {
                abbreviation = buffer.substr(0, tab_pos);
                expansion    = buffer.substr(tab_pos + 1, std::string::npos);

                logger << INFO << "Caching abbreviation: " << abbreviation << " - expansion: " << expansion << endl;
                cache[abbreviation] = expansion;
            }
        }
        
        abbr_file.close();
    }
}

void AbbreviationExpansionPredictor::update (const Observable* var)
{
    logger << DEBUG << "About to invoke dispatcher: " << var->get_name () << " - " << var->get_value() << endl;
    dispatcher.dispatch (var);
}
