
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


#ifndef PRESAGE_ASPELLPREDICTOR_H
#define PRESAGE_ASPELLPREDICTOR_H

#include "predictor.h"
#include "../core/dispatcher.h"

#include <fstream>
#include <aspell.h>
#include <memory>


/** Aspell based predictive predictor.
 *
 * Generates a prediction by running them through speller.
 *
 */
class AspellPredictor : public Predictor, public Observer {
public:
    AspellPredictor (Configuration*, ContextTracker*, const char*);
    ~AspellPredictor();

    virtual Prediction predict (const size_t size, const char** filter) const;

    virtual void learn (const std::vector<std::string>& change);

    virtual void forget(const std::string& word);

    virtual void update (const Observable* variable);

    void set_master (const std::string& value);
    void set_probability (const std::string& value);
    void set_lang (const std::string& value);


private:
    void load_speller();
    
private:
    double probability;

    Dispatcher<AspellPredictor> dispatcher;
    AspellSpeller *speller;
    AspellConfig * aspellConfig;

};

#endif // PRESAGE_ASPELLPREDICTOR_H
