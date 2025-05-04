
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


#include "dictionaryPredictor.h"

#include <assert.h>


DictionaryPredictor::DictionaryPredictor (Configuration* config, ContextTracker* ht, const char* name)
    : Predictor(config,
	     ht,
	     name,
	     "DictionaryPredictor, dictionary lookup",
	     "DictionaryPredictor, a dictionary based predictor that generates a prediction by extracting tokens that start with the current prefix from a given dictionary"
	     ),
      dispatcher (this)
{
    LOGGER      = PREDICTORS + name + ".LOGGER";
    DICTIONARY  = PREDICTORS + name + ".DICTIONARY";
    PROBABILITY = PREDICTORS + name + ".PROBABILITY";

    // build notification dispatch map
    dispatcher.map (config->find (LOGGER), & DictionaryPredictor::set_logger);
    dispatcher.map (config->find (DICTIONARY), & DictionaryPredictor::set_dictionary);
    dispatcher.map (config->find (PROBABILITY), & DictionaryPredictor::set_probability);
}

DictionaryPredictor::~DictionaryPredictor()
{
    // intentionally empty
}

void DictionaryPredictor::set_dictionary (const std::string& value)
{
    dictionary_path = value;
    logger << INFO << "DICTIONARY: " << value << endl;
}


void DictionaryPredictor::set_probability (const std::string& value)
{
    probability = Utility::toDouble (value);
    logger << INFO << "PROBABILITY: " << value << endl;
}

Prediction DictionaryPredictor::predict(const size_t max_partial_predictions_size, const char** filter) const
{
    Prediction result;

    std::string candidate;
    std::string const prefix = contextTracker->getPrefix();

    std::ifstream dictionary_file;
    dictionary_file.open(dictionary_path.c_str());
    if(!dictionary_file)
        logger << ERROR << "Error opening dictionary: " << dictionary_path << endl;
    assert(dictionary_file); // REVISIT: handle with exceptions

    std::priority_queue<Suggestion, std::vector<Suggestion>, std::greater<Suggestion>> pq;

    while (dictionary_file >> candidate) { // Reads words separated by whitespace
        std::string candidateLower{candidate};
        Utility::strtolower(candidateLower);
        int const distance = levenshteinDistance(prefix, candidateLower);
        double const cProbability = (probability) / exp(std::max(0, distance));
        Suggestion current_suggestion{candidate, cProbability};

        if (pq.size() < max_partial_predictions_size) {
            pq.push(current_suggestion);
        } else if (max_partial_predictions_size > 0 && pq.top() < current_suggestion) {
            pq.pop();
            pq.push(current_suggestion);
        }
        logger << DEBUG << "Prefix: " << prefix << " candidate: " << candidate << " distance: " << distance << " probability: " << cProbability << endl;
    }

    for (; !pq.empty(); pq.pop()) {
        logger << INFO << "Candidate: " << pq.top().getWord() << " probability: " << pq.top().getProbability()<< endl;
        result.addSuggestion(pq.top());
    }

    dictionary_file.close();

    return result;
}

void DictionaryPredictor::learn(const std::vector<std::string>& change)
{
}

void DictionaryPredictor::forget(const std::string& word)
{
}

void DictionaryPredictor::update (const Observable* var)
{
    logger << DEBUG << "About to invoke dispatcher: " << var->get_name () << " - " << var->get_value() << endl;
    dispatcher.dispatch (var);
}

/**
 * @brief Calculates the Levenshtein distance between two strings.
 *
 * The Levenshtein distance is the minimum number of single-character edits
 * (insertions, deletions, or substitutions) required to change one string
 * into the other. This implementation uses dynamic programming with O(min(m,n)) space.
 *
 * @param s1 The first string.
 * @param s2 The second string.
 * @return The Levenshtein distance between s1 and s2.
 */
int DictionaryPredictor::levenshteinDistance(const std::string& s1, const std::string& s2) const {
    const size_t len1 = s1.length();
    const size_t len2 = s2.length();

    // Optimization: use shorter string for columns to minimize space
    const std::string& shorter = (len1 < len2) ? s1 : s2;
    const std::string& longer = (len1 < len2) ? s2 : s1;
    const size_t shorter_len = shorter.length();
    const size_t longer_len = longer.length();

    if (shorter_len == 0) {
        return static_cast<int>(longer_len);
    }

    // Use only two rows for DP table (current and previous)
    std::vector<int> previous_row(shorter_len + 1);
    std::vector<int> current_row(shorter_len + 1);

    // Initialize the previous row (costs for empty 'longer' string)
    for (size_t i = 0; i <= shorter_len; ++i) {
        previous_row[i] = static_cast<int>(i);
    }
    // Or using std::iota (requires <numeric>):
    // std::iota(previous_row.begin(), previous_row.end(), 0);


    // Fill the DP table row by row
    for (size_t i = 1; i <= longer_len; ++i) {
        current_row[0] = static_cast<int>(i); // Cost for empty 'shorter' string

        for (size_t j = 1; j <= shorter_len; ++j) {
            int cost = (longer[i - 1] == shorter[j - 1]) ? 0 : 1;

            current_row[j] = std::min({
                previous_row[j] + 1,         // Deletion from longer string
                current_row[j - 1] + 1,      // Insertion into longer string
                previous_row[j - 1] + cost   // Substitution or match
            });
        }
        // Swap rows for the next iteration
        previous_row.swap(current_row);
    }

    // The final distance is in the last element of the 'previous_row'
    // (because we swapped at the end of the loop)
    return previous_row[shorter_len];
}
