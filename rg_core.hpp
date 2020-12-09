#include "rglibinc.hpp"

namespace redgene
{
    template <typename UIntType = uint_fast64_t>
    class rand_str_generator
    {
    private:
        map<UIntType, string>* rand_str_warehouse = nullptr;
        uint_fast16_t str_length = 10;
        bool bypass_warehouse = true;
        static const char alphabet[];
    public:
        
        rand_str_generator() = default;
        rand_str_generator(uint_fast16_t length, bool bypass_warehouse) :
            str_length(length), bypass_warehouse(bypass_warehouse)
        {
            if(!bypass_warehouse)
                rand_str_warehouse = new map<UIntType, string>();
        }
        string operator()(UIntType key)
        {
            string rand_str;
            if(bypass_warehouse || 
                (rand_str_warehouse->find(key) == rand_str_warehouse->end()))
            {
                rand_str.reserve(str_length);
                mt19937_64 str_prng(key);
                uniform_int_distribution<uint_fast8_t> uidist(0, 
                    sizeof(alphabet)/sizeof(*alphabet)-2);
                generate_n(back_inserter(rand_str), str_length, 
                    [&]() { return alphabet[uidist(str_prng)]; });
                if(!bypass_warehouse)
                    rand_str_warehouse->insert(pair<UIntType, string>(key, rand_str));
            }
            else
            {
                rand_str = rand_str_warehouse->find(key)->second;
            }
            return rand_str;
        }
        ~rand_str_generator()
        {
            if(rand_str_warehouse)
            {
                delete rand_str_warehouse;
            }
        }
    };

    template <>
    const char rand_str_generator<>::alphabet[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789,.-#'?!";
}