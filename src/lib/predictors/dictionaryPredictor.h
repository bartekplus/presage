
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


#ifndef PRESAGE_DICTIONARYPREDICTOR
#define PRESAGE_DICTIONARYPREDICTOR

#include "predictor.h"
#include "../core/dispatcher.h"

#include <fstream>


/** Dictionary predictive predictor.
 *
 * Generates a prediction by extracting tokens that start with the
 * current prefix from a given dictionary.
 *
 */
class DictionaryPredictor : public Predictor, public Observer {
public:
    DictionaryPredictor (Configuration*, ContextTracker*, const char*);
    ~DictionaryPredictor();

    virtual Prediction predict (const size_t size, const char** filter) const;

    virtual void learn (const std::vector<std::string>& change);

    virtual void forget(const std::string& word);

    virtual void update (const Observable* variable);

    void set_dictionary (const std::string& value);
    void set_probability (const std::string& value);

private:
    std::string LOGGER;
    std::string DICTIONARY;
    std::string PROBABILITY;

    std::string dictionary_path;
    double probability;

    Dispatcher<DictionaryPredictor> dispatcher;
    int levenshteinDistance(const std::string& s1, const std::string& s2) const;
};

#endif // PRESAGE_DICTIONARYPREDICTOR
