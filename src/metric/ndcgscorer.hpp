#ifndef __NDCG_SCORER_HPP__
#define __NDCG_SCORER_HPP__

/*! \file ndcgscorer.hpp
 * \brief Normalized Discounted Cumulative Gain (NDCG)
 */

#include <cstdio>
#include <cmath>

#include "metric/metricscorer.hpp"
#include "utils/strutils.hpp" // rtnode string
#include "utils/radix.hpp" // radix sort
#include "utils/qsort.hpp" // quick sort (for small input)

/*! \def USE_IDCGCACHE
 * enable cache for idcg values
 */
#define USE_IDCGCACHE

/*! \def POWEROFTWO
 * raise 2 to the (int) \a p-th power by means of left shift bitwise operator
 */
#define POWEROFTWO(p) ((float)(1<<((int)(p))))

/*! Compute Discounted Cumulative Gain (DCG) for a list of labels.
 * @param labels input values.
 * @param nlabels number of input values.
 * @param k maximum number of entities that can be recommended.
 * @return DCG@ \a k for computed on \a labels.
*/
float compute_dcg(float const* labels, const unsigned int nlabels, const unsigned int k) {
	unsigned int size = (k==0 or k>nlabels) ? nlabels : k;
	float dcg = 0.0f;
	#pragma omp parallel for reduction(+:dcg)
	for(unsigned int i=0; i<size; ++i)
		dcg += (POWEROFTWO(labels[i])-1.0f)/log2(i+2.0f);
	return dcg;
}

/*! Compute the ideal Discounted Cumulative Gain (iDCG) for a list of labels.
 * @param labels input values.
 * @param nlabels number of input values.
 * @param k maximum number of entities that can be recommended.
 * @return iDCG@ \a k for computed on \a labels.
*/
float compute_idcg(float const* labels, const unsigned int nlabels, const unsigned int k) {
	//make a copy of lables
	float *copyoflabels = new float[nlabels];
	memcpy(copyoflabels, labels, sizeof(float)*nlabels);
	//sort the copy
	float_qsort(copyoflabels, nlabels);
	//compute dcg
	float dcg = compute_dcg(copyoflabels, nlabels, k);
	//free mem
	delete[] copyoflabels;
	//return dcg
	return dcg;
}

/*! \class Cache ideal Discounted Cumulative Gain (iDCG) values.
*/
class idcg_cache {
	private:
		float *cache = NULL;
		unsigned int cachesize = 0;
		const float undef = -FLT_MAX;
	public:
		~idcg_cache() {
			delete [] cache;
		}
		/*! operator()
		 * \brief cache iDCG values of encountered ranklists. if \a ql.qid doesn't exist in the data structure, the related value is computed and memorized for the next utilization.
		 * @param ql input rankedlist.
		 * @param k maximum number of entities that can be recommended.
		 * @return iDCG of \a ql.
		 * \remark cached values are accessed only by their \a id value. As a consequence returned values correspond to the ones computed for the first \a k value passed.
		 */
		float operator() (const qlist &ql, const unsigned int k) {
			if(ql.qid>=cachesize || cache[ql.qid]==undef) {
				#pragma omp critical
				{
					if(ql.qid>=cachesize) {
						unsigned int newcachesize = 2*ql.qid+1;
						cache = (float*)realloc(cache,sizeof(float)*newcachesize);
						while(cachesize<newcachesize) cache[cachesize++] = undef;
					}
					cache[ql.qid] = compute_idcg(ql.labels, ql.size, k);
				}
			}
			return cache[ql.qid];
		}
};

/*! \class Cache Normalized Discounted Cumulative Gain (nDCG) values.
*/
class ndcgscorer : public metricscorer {
	public:
		/** Constructor.
		 * @param k maximum number of entities that can be recommended.
		*/
		ndcgscorer(const unsigned int kval) {
			k = kval;
		}
		/** Return a string contatining the name of the metric scorer;
		 */
		const char *whoami() const {
			return "NDCG";
		}
		/* Compute score
		 */
		float compute_score(const qlist &ql) {
			if(ql.size==0) return -1.0f;
			const unsigned int size = k<ql.size ? k : ql.size;
			#ifdef USE_IDCGCACHE
			const float idcg = ic(ql, size); //NOTE size==k in RankLib
			#else
			const float idcg = compute_idcg(ql.labels, ql.size, size); //NOTE size==k in RankLib
			#endif
			return idcg>0.0f ? compute_dcg(ql.labels, ql.size, size)/idcg : 0.0f;
		}
		/* Compute score
		 */
		fsymmatrix *swap_change(const qlist &ql) {
			const unsigned int size = k<ql.size ? k : ql.size;
			//compute the ideal ndcg
			#ifdef USE_IDCGCACHE
			const float idcg = ic(ql, size);
			#else
			const float idcg = compute_idcg(ql.labels, ql.size, size);
			#endif
			#ifdef LOGFILE
			fprintf(flog, "idcg %f :", idcg);
			for(unsigned int i=0; i<ql.size; ++i)
				fprintf(flog, " %.0f", ql.labels[i]);
			fprintf(flog, "\n");
			#endif
			fsymmatrix *changes = new fsymmatrix(ql.size);
			if(idcg>0.0f) {
				#pragma omp parallel for
				for(unsigned int i=0; i<size; ++i) {
					//get the pointer to the i-th line of matrix
					float *vchanges = changes->vectat(i, i+1);
					for(unsigned int j=i+1; j<ql.size; ++j)
						*vchanges++ = (1.0f/log2(i+2.0f)-1.0f/log2(j+2.0f))*(POWEROFTWO(ql.labels[i])-POWEROFTWO(ql.labels[j]))/idcg;
				}
			}
			return changes;
		}
	private:
		#ifdef USE_IDCGCACHE
		idcg_cache ic;
		#endif
};

#undef POWEROFTWO

#endif
