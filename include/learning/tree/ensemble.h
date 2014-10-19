#ifndef QUICKRANK_LEARNING_TREE_ENSEMBLE_H_
#define QUICKRANK_LEARNING_TREE_ENSEMBLE_H_

#include "learning/tree/rt.h"
#include "types.h"

class Ensemble {
	private:
		struct wt {
			wt(RTNode *root, float weight, float maxlabel) : root(root), weight(weight), maxlabel(maxlabel) {}
			RTNode *root = NULL;
			float weight = 0.0f;
			float maxlabel = 0.0f;
		};
		unsigned int size = 0;
		wt *arr = NULL;
	public:
		~Ensemble();
		void set_capacity(const unsigned int n);
		void push(RTNode *root, const float weight, const float maxlabel);
		void pop();

		unsigned int get_size() const {
			return size;
		}
		bool is_notempty() const {
			return size>0;
		}

		float eval(float *const *const features, unsigned int idx) const;

		// assumes vertical dataset
	  virtual qr::Score score_instance(const qr::Feature* d, const unsigned int offset=1) const;

		void write_outputtofile(FILE *f);
};

#endif
