/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include "util/check.hh"
#include <limits>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "Factor.h"
#include "Util.h"
#include "LM/SingleFactor.h"
#include "MPHR.h"
#include "KneserNeyWrapper.h"
#include <boost/shared_ptr.hpp>
#include <getopt.h>

#include "LM/Shef.h"
#include "FactorCollection.h"
#include "Phrase.h"
#include "InputFileStream.h"
#include "StaticData.h"


namespace Moses
{
namespace 
{
using namespace std;

class LanguageModelShefLM : public LanguageModelPointerState
{
public:
  LanguageModelShefLM()
    : m_lm() {}
  bool Load(const std::string &filePath, FactorType factorType, size_t nGramOrder);
  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = NULL) const;
  ~LanguageModelShefLM() {
    m_lm.reset();
  }
  uint64_t getScore(std::string *start) const; 
  /*void CleanUpAfterSentenceProcessing() {
    m_lm->clearCaches(); // clear caches
  }
  void InitializeBeforeSentenceProcessing() {
    m_lm->initThreadSpecificData(); // Creates thread specific data iff
                                    // compiled with multithreading.
  }*/
protected:
  //std::vector<randlm::WordID> m_randlm_ids_vec;
    boost::shared_ptr<MPHR> m_lm;
    
  //Shef lm doesn't use word ID 
  
  //randlm::WordID m_oov_id;
  //void CreateFactors(FactorCollection &factorCollection);
    
  /*
  randlm::WordID GetLmID( const std::string &str ) const;
  randlm::WordID GetLmID( const Factor *factor ) const {
    size_t factorId = factor->GetId();
    return ( factorId >= m_randlm_ids_vec.size()) ? m_oov_id : m_randlm_ids_vec[factorId];
  };
  */
};


bool LanguageModelShefLM::Load(const std::string &filePath, FactorType factorType,
                               size_t nGramOrder)
{
  cerr << "Loading LanguageModelShefLM..." << endl;
  //FactorCollection &factorCollection = FactorCollection::Instance();
  unsigned bits_per_fingerprint=12;
  unsigned bits_per_rank=20;
  size_t unique_bigrams=0;
  //boost::shared_ptr<MPHR> pMPHR;
    //m_lm = *pMPHR;
  m_filePath = filePath;
  m_factorType = factorType;
  m_nGramOrder = nGramOrder;
  m_lm.reset(new MPHR(filePath));  
  CHECK(m_lm != NULL);
 
  return true;
}
//commented out, ShefLM doesn't use word IDs
/*
void LanguageModelRandLM::CreateFactors(FactorCollection &factorCollection)   // add factors which have randlm id
{
  // code copied & paste from SRI LM class. should do template function
  // first get all bf vocab in map
  std::map<size_t, randlm::WordID> randlm_ids_map; // map from factor id -> randlm id
  size_t maxFactorId = 0; // to create lookup vector later on
  for(std::map<randlm::Word, randlm::WordID>::const_iterator vIter = m_lm->vocabStart();
      vIter != m_lm->vocabEnd(); vIter++) {
    // get word from randlm vocab and associate with (new) factor id
    size_t factorId=factorCollection.AddFactor(Output,m_factorType,vIter->first)->GetId();
    randlm_ids_map[factorId] = vIter->second;
    maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
  }
  // add factors for BOS and EOS and store bf word ids
  size_t factorId;
  m_sentenceStart = factorCollection.AddFactor(Output, m_factorType, m_lm->getBOS());
  factorId = m_sentenceStart->GetId();
  maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
  m_sentenceStartArray[m_factorType] = m_sentenceStart;

  m_sentenceEnd	= factorCollection.AddFactor(Output, m_factorType, m_lm->getEOS());
  factorId = m_sentenceEnd->GetId();
  maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
  m_sentenceEndArray[m_factorType] = m_sentenceEnd;

  // add to lookup vector in object
  m_randlm_ids_vec.resize(maxFactorId+1);
  // fill with OOV code
  fill(m_randlm_ids_vec.begin(), m_randlm_ids_vec.end(), m_oov_id);

  for (map<size_t, randlm::WordID>::const_iterator iter = randlm_ids_map.begin();
       iter != randlm_ids_map.end() ; ++iter)
    m_randlm_ids_vec[iter->first] = iter->second;

}

randlm::WordID LanguageModelRandLM::GetWord( const std::string &str ) const
{
  return str->GetString();
}*/

LMResult LanguageModelShefLM::GetValue(const vector<const Word*> &contextFactor,
                                    State* finalState) const
{
  FactorType factorType = GetFactorType();
  // set up context
  string ngram[MAX_NGRAM_SIZE];
  int count = contextFactor.size();
  for (int i = 0 ; i < count ; i++) {
    ngram[i] = ((*contextFactor[i])[factorType])->GetString();
      cout << ngram[i];
    //std::cerr << m_lm->getWord(ngram[i]) << " ";
  }
  //int found = 0;
  LMResult ret;
    //start of stupid backoff
    float prob = 0;
    float smoothing = 0.4; //set smoothing to constant at the moment
    for (unsigned int i=0; i<(count-m_nGramOrder); i++) 
    {
        if (getScore(&ngram[i])>0)
        {
            prob = (getScore(&ngram[i]) / getScore(&ngram[i+1])) * pow(smoothing, (signed int)i);
            break;
        }
    }
    //end of stupid backoff
    
  ret.score = FloorScore(TransformLMScore(prob));
  //ret.unknown = count && (ngram[count - 1] == m_oov_id);
  //if (finalState)
  //  std::cerr << " = " << logprob << "(" << *finalState << ", " <<")"<< std::endl;
  //else
  //  std::cerr << " = " << logprob << std::endl;
  return ret;
}
                         
uint64_t LanguageModelShefLM::getScore(std::string *start) const
{
    std::string ngrams;
 for (std::string *i = start; i != start + m_nGramOrder && *i != ""; ++i)
 {
     ngrams += *i;
 }
 return m_lm->query(ngrams);
}
                                             

}

LanguageModelPointerState *NewShefLM() {
  return new LanguageModelShefLM();
}
                         }                         


