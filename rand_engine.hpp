#include "rglibinc.hpp"

namespace redgene
{
    typedef enum random_engines
    {
        DEFAULT=1,
        MINSTD_RAND0,
        MINSTD_RAND1,
        MT19937,
        MT19937_64,
        RANLUX24,
        RANLUX48
    } random_engines;

    //Abstract Base Class template
    template <typename UIntTypes>
    class prng_base
    {
    public:
        virtual void seed(UIntTypes seed) = 0;
        virtual UIntTypes operator()() = 0;
        virtual void discard(unsigned long long t) = 0;
        virtual UIntTypes min() = 0;
        virtual UIntTypes max() = 0;
        virtual ~prng_base() = default;
    };

    template <typename UIntTypes>
    class default_prng : public prng_base<UIntTypes>
    {
    private:
        default_random_engine* def_prng;
    public:
        default_prng(UIntTypes seed = default_random_engine::default_seed) : 
            def_prng(new default_random_engine(seed))
        {

        }

        ~default_prng()
        {
            delete def_prng;
        }

        void seed(UIntTypes seed)
        {
            def_prng->seed(seed);
        }

        inline UIntTypes operator()()
        {
            return (*def_prng)();
        }

        void discard(unsigned long long t)
        {
            def_prng->discard(t);
        }

        UIntTypes min()
        {
            return default_random_engine::min();
        }

        UIntTypes max()
        {
            return default_random_engine::max();
        }
    };

    template <typename UIntTypes>
    class minstd0_prng : public prng_base<UIntTypes>
    {
    private:
        minstd_rand0* mstd0_prng;
    public:
        minstd0_prng(UIntTypes seed = minstd_rand0::default_seed) : 
            mstd0_prng(new minstd_rand0(seed))
        {

        }
        ~minstd0_prng()
        {
            delete mstd0_prng;
        }
        void seed(UIntTypes seed)
        {
            mstd0_prng->seed(seed);
        }

        inline UIntTypes operator()()
        {
            return (*mstd0_prng)();
        }

        void discard(unsigned long long t)
        {
            mstd0_prng->discard(t);
        }

        UIntTypes min()
        {
            return minstd_rand0::min();
        }

        UIntTypes max()
        {
            return minstd_rand0::max();
        }
    };

    template <typename UIntTypes>
    class minstd1_prng : public prng_base<UIntTypes>
    {
    private:
        minstd_rand* mstd_prng;
    public:
        minstd1_prng(UIntTypes seed = minstd_rand::default_seed) : 
            mstd_prng(new minstd_rand(seed))
        {

        }
        ~minstd1_prng()
        {
            delete mstd_prng;
        }
        void seed(UIntTypes seed)
        {
            mstd_prng->seed(seed);
        }

        inline UIntTypes operator()()
        {
            return (*mstd_prng)();
        }

        void discard(unsigned long long t)
        {
            mstd_prng->discard(t);
        }

        UIntTypes min()
        {
            return minstd_rand::min();
        }

        UIntTypes max()
        {
            return minstd_rand::max();
        }
    };

    template <typename UIntTypes>
    class mt19937_prng : public prng_base<UIntTypes>
    {
    private:
        mt19937* mt_prng;
    public:
        mt19937_prng(UIntTypes seed = mt19937::default_seed) : 
            mt_prng(new mt19937(seed))
        {

        }
        ~mt19937_prng()
        {
            delete mt_prng;
        }
        void seed(UIntTypes seed)
        {
            mt_prng->seed(seed);
        }

        inline UIntTypes operator()()
        {
            return (*mt_prng)();
        }

        void discard(unsigned long long t)
        {
            mt_prng->discard(t);
        }

        UIntTypes min()
        {
            return mt19937::min();
        }

        UIntTypes max()
        {
            return mt19937::max();
        }
    };

    template <typename UIntTypes>
    class ranlux24_prng : public prng_base<UIntTypes>
    {
    private:
        ranlux24_base* rl24_prng;
    public:
        ranlux24_prng(UIntTypes seed = ranlux24_base::default_seed) : 
            rl24_prng(new ranlux24_base(seed))
        {

        }
        ~ranlux24_prng()
        {
            delete rl24_prng;
        }
        void seed(UIntTypes seed)
        {
            rl24_prng->seed(seed);
        }

        inline UIntTypes operator()()
        {
            return (*rl24_prng)();
        }

        void discard(unsigned long long t)
        {
            rl24_prng->discard(t);
        }

        UIntTypes min()
        {
            return ranlux24_base::min();
        }

        UIntTypes max()
        {
            return ranlux24_base::max();
        }
    };

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Woverflow"

    template <typename UIntTypes>
    class mt19937_64_prng : public prng_base<UIntTypes>
    {
    private:
        mt19937_64* mt64_prng;
    public:
        mt19937_64_prng(UIntTypes seed = mt19937_64::default_seed) : 
            mt64_prng(new mt19937_64(seed))
        {

        }
        ~mt19937_64_prng()
        {
            delete mt64_prng;
        }
        void seed(UIntTypes seed)
        {
            mt64_prng->seed(seed);
        }

        inline UIntTypes operator()()
        {
            return (*mt64_prng)();
        }

        void discard(unsigned long long t)
        {
            mt64_prng->discard(t);
        }

        UIntTypes min()
        {
            return mt19937_64::min();
        }

        UIntTypes max()
        {
            return mt19937_64::max();
        }
    };

    template <typename UIntTypes>
    class ranlux48_prng : public prng_base<UIntTypes>
    {
    private:
        ranlux48_base* rl48_prng;
    public:
        ranlux48_prng(UIntTypes seed = ranlux48_base::default_seed) : 
            rl48_prng(new ranlux48_base(seed))
        {

        }
        ~ranlux48_prng()
        {
            delete rl48_prng;
        }
        void seed(UIntTypes seed)
        {
            rl48_prng->seed(seed);
        }

        inline UIntTypes operator()()
        {
            return (*rl48_prng)();
        }

        void discard(unsigned long long t)
        {
            rl48_prng->discard(t);
        }

        UIntTypes min()
        {
            return ranlux48_base::min();
        }

        UIntTypes max()
        {
            return ranlux48_base::max();
        }
    };

    #pragma GCC diagnostic pop

    template <typename UIntTypes>
    class prng_engine
    {
    private:
        prng_base<UIntTypes>* p1;
    public:
        using result_type = UIntTypes;
        prng_engine(random_engines engine)
        {
            switch(engine)
            {
                case DEFAULT:
                    p1 = new default_prng<UIntTypes>();
                    break;
                case MINSTD_RAND0:
                    p1 = new minstd0_prng<UIntTypes>();
                    break;
                case MINSTD_RAND1:
                    p1 = new minstd1_prng<UIntTypes>();
                    break;
                case MT19937:
                    p1 = new mt19937_prng<UIntTypes>();
                    break;
                case MT19937_64:
                    p1 = new mt19937_64_prng<UIntTypes>();
                    break;
                case RANLUX24:
                    p1 = new ranlux24_prng<UIntTypes>();
                    break;
                case RANLUX48:
                    p1 = new ranlux48_prng<UIntTypes>();
                    break;
                default:
                    p1 = new default_prng<UIntTypes>();
            }
        }
        prng_engine(random_engines engine, UIntTypes seed)
        {
            switch(engine)
            {
                case DEFAULT:
                    p1 = new default_prng<UIntTypes>(seed);
                    break;
                case MINSTD_RAND0:
                    p1 = new minstd0_prng<UIntTypes>(seed);
                    break;
                case MINSTD_RAND1:
                    p1 = new minstd1_prng<UIntTypes>(seed);
                    break;
                case MT19937:
                    p1 = new mt19937_prng<UIntTypes>(seed);
                    break;
                case MT19937_64:
                    p1 = new mt19937_64_prng<UIntTypes>(seed);
                    break;
                case RANLUX24:
                    p1 = new ranlux24_prng<UIntTypes>(seed);
                    break;
                case RANLUX48:
                    p1 = new ranlux48_prng<UIntTypes>(seed);
                    break;
                default:
                    p1 = new default_prng<UIntTypes>(seed);
            }
        }
        void seed(UIntTypes seed)
        {
            p1->seed(seed);
        }
        inline UIntTypes operator()()
        {
            return (*p1)();
        }
        void discard(unsigned long long t)
        {
            p1->discard(t);
        }
        UIntTypes min()
        {
            return p1->min();
        }
        UIntTypes max()
        {
            return p1->max();
        }
        ~prng_engine()
        {
            delete p1;
        }
    };
}
