/*
 * QuickRank - A C++ suite of Learning to Rank algorithms
 * Webpage: http://quickrank.isti.cnr.it/
 * Contact: quickrank@isti.cnr.it
 *
 * Unless explicitly acquired and licensed from Licensor under another
 * license, the contents of this file are subject to the Reciprocal Public
 * License ("RPL") Version 1.5, or subsequent versions as allowed by the RPL,
 * and You may not copy or use this file in either source code or executable
 * form, except in compliance with the terms and conditions of the RPL.
 *
 * All software distributed under the RPL is provided strictly on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND
 * LICENSOR HEREBY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT
 * LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, QUIET ENJOYMENT, OR NON-INFRINGEMENT. See the RPL for specific
 * language governing rights and limitations under the RPL.
 *
 * Contributor:
 *   HPC. Laboratory - ISTI - CNR - http://hpc.isti.cnr.it/
 */
#include <iostream>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <sys/stat.h>
#include <list>

#if defined(_WIN32) || defined(__CYGWIN__) 
// if typedef doesn't exist (msvc, blah)
typedef intptr_t ssize_t;

ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
    size_t pos;
    int c;

    if (lineptr == NULL || stream == NULL || n == NULL) {
        errno = EINVAL;
        return -1;
    }

    c = fgetc(stream);
    if (c == EOF) {
        return -1;
    }

    if (*lineptr == NULL) {
        *lineptr = (char*)malloc(128);
        if (*lineptr == NULL) {
            return -1;
        }
        *n = 128;
    }

    pos = 0;
    while(c != EOF) {
        if (pos + 1 >= *n) {
            size_t new_size = *n + (*n >> 2);
            if (new_size < 128) {
                new_size = 128;
            }
            char *new_ptr = (char*)realloc(*lineptr, new_size);
            if (new_ptr == NULL) {
                return -1;
            }
            *n = new_size;
            *lineptr = new_ptr;
        }

        ((unsigned char *)(*lineptr))[pos ++] = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }

    (*lineptr)[pos] = '\0';
    return pos;
}
#endif

#include "io/svml.h"
#include "utils/strutils.h"


namespace quickrank {
namespace io {

// TODO: save info file or use mmap
// TODO: re-introduce multithreading
std::unique_ptr<data::Dataset> Svml::read_horizontal(
    const std::string &filename) {

  //FILE *f = fopen(filename.c_str(), "r");
  std::ifstream f(filename);

  if (!f) {
    std::cerr << "!!! Error while opening file " << filename << "."
              << std::endl;
    exit(EXIT_FAILURE);
  }

  struct stat filestatus;
  stat(filename.c_str(), &filestatus);
  file_size_ = filestatus.st_size;

  std::chrono::high_resolution_clock::time_point start_reading =
      std::chrono::high_resolution_clock::now();

  size_t maxfid = 0;

  // temporary copy of data
  std::list<size_t> data_qids;
  std::list<quickrank::Label> data_labels;
  std::list<std::vector<quickrank::Feature>> data_instances;

  // Alloc 10kb on stack for each line,
  // just to be on safe side for bigger input files.
  char line[1024*10];
  std::vector<quickrank::Feature> curr_instance;
  curr_instance.reserve(128);
  while (f.getline( line, sizeof line )) {
    if ( (f.rdstate() & std::ifstream::failbit ) != 0 ) {
      std::cerr << "!!! Failed to read input line (probably bigger than 10k) "
                << filename << "." << std::endl;
      exit(3);
    }

    char *token = NULL, *pch = line;
    //skip initial spaces
    while (ISSPC(*pch) && *pch != '\0')
      ++pch;
    //skip comment line
    if (*pch == '#') {
      continue;
    }

    //read label (label is a mandatory field)
    if (ISEMPTY(token = read_token(pch)))
      exit(2);

    // read label and qid
    quickrank::Label relevance = atof(token);
    size_t qid = atou(read_token(pch), "qid:");

    // Clean feature vector with 0s
    curr_instance.clear();
    curr_instance.resize(maxfid);

    //read a sequence of features, namely (fid,fval) pairs, then the ending description
    while (!ISEMPTY(token = read_token(pch, '#'))) {
      if (*token == '#') {
//#ifndef SKIP_DPDESCRIPTION
//        datapoint->set_description(strdup(++token));
//#endif
        *pch = '\0';
      } else {
        //read a feature (id,val) from token
        size_t fid = 0;
        float fval = 0.0f;
        if (sscanf(token, "%zu:%f", &fid, &fval) != 2)
          exit(4);
        //add feature to the current dp
        if (fid > maxfid) {
          maxfid = fid;
          curr_instance.resize(maxfid);
        }
        curr_instance[fid - 1] = fval;
      }
    }

    // store partial data
    //#pragma omp ordered
    {
      data_qids.push_back(qid);
      data_labels.push_back(relevance);
      data_instances.push_back(std::move(curr_instance));  // move should avoid
      // copies
    }
  }
  //close input file
  f.close();

  std::chrono::high_resolution_clock::time_point start_processing =
      std::chrono::high_resolution_clock::now();

  // put partial data in final data structure
  data::Dataset *dataset = new data::Dataset(data_qids.size(), maxfid);
  auto i_q = data_qids.begin();
  auto i_l = data_labels.begin();
  auto i_x = data_instances.begin();
  while (i_q != data_qids.end()) {
    dataset->addInstance(*i_q, *i_l, std::move(*i_x));
    i_q++;
    i_l++;
    i_x++;
  }

  std::chrono::high_resolution_clock::time_point end_processing =
      std::chrono::high_resolution_clock::now();

  reading_time_ = std::chrono::duration_cast<std::chrono::duration<double>>(
      start_processing - start_reading).count();

  processing_time_ = std::chrono::duration_cast<std::chrono::duration<double>>(
      end_processing - start_processing).count();

  return std::unique_ptr<data::Dataset>(dataset);
}

void Svml::write(std::shared_ptr<data::Dataset> dataset,
                 const std::string &file) {

  std::ofstream outFile(file, std::ofstream::out | std::ofstream::trunc);

  for (size_t q = 0; q < dataset->num_queries(); q++) {
    std::shared_ptr<data::QueryResults> results = dataset->getQueryResults(q);
    const Feature *features = results->features();
    const Label *labels = results->labels();

    for (size_t r = 0; r < results->num_results(); r++) {
      outFile << std::setprecision(0) << labels[r] << " qid:" << q + 1;
      for (size_t f = 0; f < dataset->num_features(); f++) {
        outFile << " " << f + 1 << ":"
                << std::fixed
                << std::setprecision(
                    std::numeric_limits<quickrank::Feature>::max_digits10)
                << features[f];
      }
      outFile << std::endl;
      features += dataset->num_features();
    }
  }

  outFile.close();
}

std::ostream &Svml::put(std::ostream &os) const {
  // num threads is not reported here.
  os << std::setprecision(2) << "#\t Reading time: " << reading_time_
     << " s. @ " << file_size_ / 1024 / 1024 / reading_time_ << " MB/s "
     << " (post-proc.: " << processing_time_ << " s.)" << std::endl;
  return os;
}

}  // namespace data
}  // namespace quickrank
