#include "rglibinc.hpp"

namespace redgene
{
    template <typename UIntType = uint_fast64_t>
    class rand_str_generator
    {
    private:
        map<UIntType, string>* rand_str_warehouse = nullptr;
        uint_fast16_t str_length = 10;
        bool is_variable_length = false;
        bool bypass_warehouse = true;
        static const char alphabet[];
        uint_fast16_t var_length_base = 6;

        //moved from the call operator
        mt19937_64* str_prng;
        uniform_int_distribution<uint_fast16_t>* rand_var_len = nullptr;
        uniform_int_distribution<uint_fast8_t>* uidist = nullptr;
        string* rand_str = nullptr;
    public:
        
        rand_str_generator() = default;
        rand_str_generator(uint_fast16_t length, bool is_var_length = false, 
            bool bypass_warehouse = true) :
            str_length(length), is_variable_length(is_var_length), 
            bypass_warehouse(bypass_warehouse)
        {
            if(!bypass_warehouse)
                rand_str_warehouse = new map<UIntType, string>();
            if(is_variable_length && str_length <= 6)
                is_variable_length = false;
            
            str_prng = new mt19937_64;
            rand_var_len = new uniform_int_distribution<uint_fast16_t>(var_length_base, str_length);
            uidist = new uniform_int_distribution<uint_fast8_t>(0, sizeof(alphabet)/sizeof(*alphabet)-2);
            rand_str = new string(str_length, ' ');
        }
        string operator()(UIntType key)
        {
            rand_str->clear();
            uint_fast16_t cond_str_length = str_length;
            if(bypass_warehouse || 
                (rand_str_warehouse->find(key) == rand_str_warehouse->end()))
            {
                str_prng->seed(key);
                if(is_variable_length)
                {
                    cond_str_length = (*rand_var_len)(*str_prng);
                }
                
                for(uint_fast16_t i = 0; i < cond_str_length; ++i)
                    *rand_str += std::move(alphabet[(*uidist)(*str_prng)]);

                if(!bypass_warehouse)
                    rand_str_warehouse->insert(pair<UIntType, string>(key, *rand_str));
            }
            else
            {
                *rand_str = rand_str_warehouse->find(key)->second;
            }
            return *rand_str;
        }
        ~rand_str_generator()
        {
            if(rand_str_warehouse)
                delete rand_str_warehouse;
            if(uidist)
                delete uidist;
            if(rand_var_len)
                delete rand_var_len;
            if(str_prng)
                delete str_prng;
            if(rand_str)
                delete rand_str;
        }
    };

    template <>
    const char rand_str_generator<>::alphabet[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789,.-#'?!";
}