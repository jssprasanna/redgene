#include "rglibinc.hpp"

namespace redgene
{
    template <typename disttype>
    class prob_dist_base
    {
    public:
        virtual void reset() = 0;
        virtual disttype operator()() = 0;
        virtual disttype min() = 0;
        virtual disttype max() = 0;
        virtual ~prob_dist_base() = default;
    };

    template <typename prngtype = uint_fast64_t, typename disttype = int>
    class uniform_int_dist_engine : public prob_dist_base<disttype>
    {
    private:
        prng_engine<prngtype>& prng;
        uniform_int_distribution<disttype>* uni_int_dist;  
    public:
        uniform_int_dist_engine(prng_engine<prngtype>& prng, disttype a = 0, 
            disttype b = std::numeric_limits<disttype>::max()) : prng(prng),
            uni_int_dist(new uniform_int_distribution<disttype>(a, b))
        {
        
        }

        ~uniform_int_dist_engine()
        {
            delete uni_int_dist;
        }

        void reset()
        {
            uni_int_dist->reset();
        }

        disttype operator()()
        {
            return (*uni_int_dist)(prng);
        }

        disttype min()
        {
            return uni_int_dist->min();
        }

        disttype max()
        {
            return uni_int_dist->max();
        }
    };

    template <typename prngtype = uint_fast64_t, typename disttype = double>
    class uniform_real_dist_engine : public prob_dist_base<disttype>
    {
    private:
        prng_engine<prngtype>& prng;
        uniform_real_distribution<disttype>* uni_real_dist;
    public:
        uniform_real_dist_engine(prng_engine<prngtype>& prng, disttype a = 0.0,
            disttype b = 1.0) : prng(prng), 
            uni_real_dist(new uniform_real_distribution<disttype>(a, b))
        {

        }

        ~uniform_real_dist_engine()
        {
            delete uni_real_dist;
        }

        void reset()
        {
            uni_real_dist->reset();
        }

        disttype operator()()
        {
            return (*uni_real_dist)(prng);
        }

        disttype min()
        {
            return uni_real_dist->min();
        }

        disttype max()
        {
            return uni_real_dist->max();
        }
    };

    //TODO
    //ADD zipfian distribution
    //ADD set distribution
}
