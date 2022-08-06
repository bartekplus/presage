
/******************************************************
 *  Presage, an extensible predictive text entry system
 *  ---------------------------------------------------
 *
 *  AspellPredictor: Copyright (C) 2020 Bartosz Tomczyk https://github.com/bartekplus

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


#include "aspellPredictor.h"

#include <assert.h>
#include <aspell.h>


AspellPredictor::AspellPredictor (Configuration* config, ContextTracker* ht, const char* name)
    : Predictor(config,
	     ht,
	     name,
	     "AspellPredictor, ASPELL predictor",
	     "AspellPredictor, ASPELL based predictor"
	     ),
      dispatcher (this),
      speller(nullptr),
      aspellConfig(new_aspell_config())
{
    std::string LOGGER          = PREDICTORS + name + ".LOGGER";
    std::string MASTER  = PREDICTORS + name + ".MASTER";
    std::string PROBABILITY     = PREDICTORS + name + ".PROBABILITY";
    std::string LANG            = PREDICTORS + name + ".LANG";

    // build notification dispatch map
    dispatcher.map (config->find (LOGGER), & AspellPredictor::set_logger);
    dispatcher.map (config->find (MASTER, ""), & AspellPredictor::set_master);
    dispatcher.map (config->find (PROBABILITY), & AspellPredictor::set_probability);
    dispatcher.map (config->find (LANG, ""), & AspellPredictor::set_lang);

    aspell_config_replace(aspellConfig, "encoding", "utf-8");
    load_speller();
}

AspellPredictor::~AspellPredictor()
{
    delete_aspell_config(aspellConfig);
    delete_aspell_speller(speller);
}

void AspellPredictor::set_master (const std::string& value)
{
    if (!value.empty())
    {
      aspell_config_replace(aspellConfig, "master", value.c_str());
      logger << INFO << "MASTER: " << value << endl;
    }
}

void AspellPredictor::set_probability (const std::string& value)
{
    probability = Utility::toDouble (value);
    logger << INFO << "PROBABILITY: " << value << endl;
}

void AspellPredictor::set_lang (const std::string& value)
{
    if (!value.empty())
    {
      aspell_config_replace(aspellConfig, "lang", value.c_str());
      logger << INFO << "LANG: " << value << endl;
    }
}

void AspellPredictor::load_speller()
{
  AspellCanHaveError * ret;
  ret = new_aspell_speller(aspellConfig);
  if (aspell_error(ret) != 0) {
    std::string errorStr(aspell_error_message(ret));
    delete_aspell_can_have_error(ret);
    throw PresageException(PRESAGE_ERROR, errorStr);
  }
  speller = to_aspell_speller(ret);
}

Prediction AspellPredictor::predict(const size_t max_partial_predictions_size, const char** filter) const
{
    Prediction result;

    std::string word
     = contextTracker->getPrefix();

    if (!speller || word.empty())
      return result;


    if (aspell_speller_check(speller,  word.c_str(), word.length()) != 1)
    {
      const AspellWordList *wl = aspell_speller_suggest(speller, word.c_str(), word.length());
      if (wl)
      {
        AspellStringEnumeration * els = aspell_word_list_elements(wl);
        const char * word;
        unsigned idx;
        while ( (word = aspell_string_enumeration_next(els)) != 0) {
          result.addSuggestion(Suggestion(word, probability / ++idx));
        }
        delete_aspell_string_enumeration(els);
      }
    }

    return result;
}

void AspellPredictor::learn(const std::vector<std::string>& change)
{
}

void AspellPredictor::forget(const std::string& word)
{
}

void AspellPredictor::update (const Observable* var)
{
    logger << DEBUG << "About to invoke dispatcher: " << var->get_name () << " - " << var->get_value() << endl;
    dispatcher.dispatch (var);
}
