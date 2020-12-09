#include "rglibinc.hpp"

namespace redgene
{
    template <typename disttype>
    class prob_dist_base
    {
    public:
        virtual void reset() = 0;
        virtual disttype operator()() = 0;
        virtual disttype min() const = 0;
        virtual disttype max() const = 0;
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

        disttype min() const
        {
            return uni_int_dist->min();
        }

        disttype max() const
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

        disttype min() const
        {
            return uni_real_dist->min();
        }

        disttype max() const
        {
            return uni_real_dist->max();
        }
    };

    //ADD zipfian distribution
    template<typename prngtype = uint_fast64_t, class disttype = uint_fast64_t>
    class zipf_distribution : public prob_dist_base<disttype>
    {
    private:
        prng_engine<prngtype>& prng;
        disttype    n;     ///< Number of elements
        double   _s;    ///< Exponent
        double   _q;    ///< Deformation
        double   oms;   ///< 1-s
        bool       spole; ///< true if s near 1.0
        double   rvs;   ///< 1/(1-s)
        double   H_x1;  ///< H(x_1)
        double   H_n;   ///< H(n)
        double   cut;   ///< rejection cut
        std::uniform_real_distribution<double> dist;  ///< [H(x_1), H(n)]

        // This provides 16 decimal places of precision,
        // i.e. good to (epsilon)^4 / 24 per expanions log, exp below.
        static constexpr double epsilon = 2e-5;

        /** (exp(x) - 1) / x */
        static double
        expxm1bx(const double x)
        {
            if (std::abs(x) > epsilon)
                return std::expm1(x) / x;
            return (1.0 + x/2.0 * (1.0 + x/3.0 * (1.0 + x/4.0)));
        }

        /** log(1 + x) / x */
        static double
        log1pxbx(const double x)
        {
            if (std::abs(x) > epsilon)
                return std::log1p(x) / x;
            return 1.0 - x * ((1/2.0) - x * ((1/3.0) - x * (1/4.0)));
        }
        /**
         * The hat function h(x) = 1/(x+q)^s
         */
        const double h(const double x)
        {
            return std::pow(x + _q, -_s);
        }

        /**
         * H(x) is an integral of h(x).
         *     H(x) = [(x+q)^(1-s) - (1+q)^(1-s)] / (1-s)
         * and if s==1 then
         *     H(x) = log(x+q) - log(1+q)
         *
         * Note that the numerator is one less than in the paper
         * order to work with all s. Unfortunately, the naive
         * implementation of the above hits numerical underflow
         * when q is larger than 10 or so, so we split into
         * different regimes.
         *
         * When q != 0, we shift back to what the paper defined:

        *    H(x) = (x+q)^{1-s} / (1-s)
        * and for q != 0 and also s==1, use
        *    H(x) = [exp{(1-s) log(x+q)} - 1] / (1-s)
        */
        const double H(const double x)
        {
            if (not spole)
                return std::pow(x + _q, oms) / oms;

            const double log_xpq = std::log(x + _q);
            return log_xpq * expxm1bx(oms * log_xpq);
        }

        /**
         * The inverse function of H(x).
         *    H^{-1}(y) = [(1-s)y + (1+q)^{1-s}]^{1/(1-s)} - q
         * Same convergence issues as above; two regimes.
         *
         * For s far away from 1.0 use the paper version
         *    H^{-1}(y) = -q + (y(1-s))^{1/(1-s)}
         */
        const double H_inv(const double y)
        {
            if (not spole)
                return std::pow(y * oms, rvs) - _q;

            return std::exp(y * log1pxbx(oms * y)) - _q;
        }
    public:
        typedef disttype result_type;

        static_assert(std::numeric_limits<disttype>::is_integer, "");
        static_assert(!std::numeric_limits<double>::is_integer, "");

        /// zipf_distribution(N, s, q)
        /// Zipf distribution for `N` items, in the range `[1,N]` inclusive.
        /// The distribution follows the power-law 1/(n+q)^s with exponent
        /// `s` and Hurwicz q-deformation `q`.
        zipf_distribution(prng_engine<prngtype>& prng, 
            const disttype n=std::numeric_limits<disttype>::max(),
            const double s=1.0, const double q=0.0)
            : prng(prng), n(n), _s(s), _q(q), oms(1.0-s) , spole(abs(oms) < epsilon)
            , rvs(spole ? 0.0 : 1.0/oms), H_x1(H(1.5) - h(1.0)), H_n(H(n + 0.5))
            , cut(1.0 - H_inv(H(1.5) - h(1.0))), dist(H_x1, H_n)
        {
            if (-0.5 >= q)
                throw std::runtime_error("Range error: Parameter q must be greater than -0.5!");
        }

        void reset()
        {

        }

        disttype operator()()
        {
            while (true)
            {
                const double u = dist(prng);
                const double x = H_inv(u);
                const disttype  k = std::round(x);
                if (k - x <= cut) return k;
                if (u >= H(k + 0.5) - h(k))
                return k;
            }
        }

        /// Returns the parameter the distribution was constructed with.
        double s() const { return _s; }
        /// Returns the Hurwicz q-deformation parameter.
        double q() const { return _q; }
        /// Returns the minimum value potentially generated by the distribution.
        result_type min() const { return 1; }
        /// Returns the maximum value potentially generated by the distribution.
        result_type max() const { return n; }
    };

    //TODO
    //ADD set distribution
}
