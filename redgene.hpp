#include "rglibinc.hpp"
#include "rand_engine.hpp"
#include "prob_dist.hpp"
#include "rg_utils.hpp"
#include "json.hpp"
#include <type_traits>

using namespace redgene;
using json = nlohmann::json;

namespace redgene
{
    //reference zipf alpha value: NO = NA, LOW = 0.5, MEDIUM = 0.9, HIGH = 1.1, EXTREME = 1.5
    using skewness = enum { NO = 1, LOW, MEDIUM, HIGH, EXTREME };

    const float get_alpha_value(const skewness skew)
    {
        float alpha;
        switch(skew)
        {
            case skewness::LOW:
                alpha = 0.5;
                break;
            case skewness::MEDIUM:
                alpha = 0.9;
                break;
            case skewness::HIGH:
                alpha = 1.2;
                break;
            case skewness::EXTREME:
                alpha = 1.5;
                break;
            default:
                alpha = 0;
        }
        return alpha;
    }

    //REDGENE VALIDATOR CLASS
    class redgene_validator
    {
    private:
        json redgene_json;
        bool valid = false;
        static const set<string> valid_types;
        static const set<string> valid_constraints;
        static const set<string> valid_skewness;
        bool validate();
    public:
        redgene_validator() = delete;
        redgene_validator(const string& json_filename);
        bool is_valid();
    };

    const set<string> redgene_validator::valid_types = {"INT", "REAL", "STRING"};
    const set<string> redgene_validator::valid_constraints = {"PK", "FK", "COMP_PK", "COMP_FK", "FK_UNIQUE"};
    const set<string> redgene_validator::valid_skewness = {"NO", "LOW", "MEDIUM", "HIGH", "EXTREME"};

    redgene_validator::redgene_validator(const string& json_filename)
    {
        try
        {
            ifstream ifstrm(json_filename);
            ifstrm >> redgene_json;
            valid = validate();
        }
        catch(json::parse_error& e)
        {
            redgene_json = NULL;
            // output exception information
            cout << "message: " << e.what() << '\n'
                << "exception id: " << e.id << '\n'
                << "byte position of error: " << e.byte << endl;
        }
        catch(const std::exception& e)
        {
            cerr << e.what() << endl;
        }  
    }

    bool redgene_validator::is_valid()
    {
        return valid;
    }

    bool redgene_validator::validate()
    {
        set<string> aux_tab_names_set;
        auto tab_arr_obj = redgene_json.find("tables");
        if(tab_arr_obj == redgene_json.end())
            return false;
        
        //Table Names Validation
        for(auto table_obj : tab_arr_obj.value())
        {
            auto table_name = table_obj.find("table_name");
            if(table_name == table_obj.end())
                return false;
            auto table_name_str = table_name.value().get<string>();
            transform(table_name_str.begin(), table_name_str.end(),
                table_name_str.begin(), ::tolower);
            aux_tab_names_set.insert(table_name_str);

            //Row Count Validation
            auto row_count = table_obj.find("row_count");
            if(row_count == table_obj.end())
                return false;

            if(!row_count.value().is_number_unsigned())
                return false;

            //Column Names Validation
            set<string> aux_col_names_set;
            auto column_arr_obj = table_obj.find("columns");
            if(column_arr_obj == table_obj.end())
                return false;
            
            for(auto column_obj : column_arr_obj.value())
            {
                auto column_name = column_obj.find("column_name");
                if(column_name == column_obj.end())
                    return false;
                auto col_name_str = column_name.value().get<string>();
                transform(col_name_str.begin(), col_name_str.end(),
                    col_name_str.begin(), ::tolower);
                aux_col_names_set.insert(col_name_str);

                //Check for 'CONSTRAINT', 'CARDINALITY' and 'TYPE'
                auto cardinality = column_obj.find("cardinality");

                if(cardinality != column_obj.end())
                {
                    if(cardinality.value().get<float>() <= 0.0)
                        return false;
                }

                auto constraint = column_obj.find("constraint");
                //Logic to check if inavlid constraint value is provided.
                if(constraint != column_obj.end())
                {
                    auto constraint_val = constraint.value().get<string>();
                    transform(constraint_val.begin(), constraint_val.end(),
                        constraint_val.begin(), ::toupper);
                    
                    if(valid_constraints.find(constraint_val) == valid_constraints.end())
                        return false;
                }

                auto col_data_type = column_obj.find("type");
                auto skewness = column_obj.find("skewness");
                //Logic to check if invalid skewness value is provided
                if(skewness != column_obj.end())
                {
                    auto skewness_val = skewness.value().get<string>();
                    transform(skewness_val.begin(), skewness_val.end(),
                        skewness_val.begin(), ::toupper);

                    if(valid_skewness.find(skewness_val) == valid_skewness.end())
                        return false;
                }
                
                if(col_data_type != column_obj.end())
                {
                    auto type = col_data_type.value().get<string>();
                    transform(type.begin(), type.end(), type.begin(), ::toupper);

                    if(valid_types.find(type) == valid_types.end())
                        return false;

                    if(type == "STRING")
                    {
                        auto str_length = column_obj.find("length");
                        if(str_length != column_obj.end())
                            if(str_length.value().get<int_fast16_t>() <= 0 || str_length.value().get<int_fast16_t>() > 4000)
                                return false;
                    }

                    if(type == "REAL")
                    {
                        if(constraint != column_obj.end() || cardinality != column_obj.end())
                            return false;
                        float real_min = 0.0, real_max = 1.0;
                        auto real_min_obj = column_obj.find("real_min");
                        auto real_max_obj = column_obj.find("real_max");

                        if(real_min_obj != column_obj.end())
                            real_min = real_min_obj.value().get<float>();
                            
                        if(real_max_obj != column_obj.end())
                            real_max = real_max_obj.value().get<float>();
                        
                        if(real_min >= real_max)
                            return false;
                    }
                    else
                    {
                        if(!(constraint != column_obj.end() || cardinality != column_obj.end()))
                            return false;
                    }
                }
                else
                {
                    if(!(constraint != column_obj.end() || cardinality != column_obj.end()))
                        return false;
                }
            }
            if(aux_col_names_set.size() != column_arr_obj.value().size())
                return false;
        }

        if(aux_tab_names_set.size() != tab_arr_obj.value().size())
            return false;

        return true;
    }


    //TABLE ATTRIBUTES SECTION
    class table_type
    {
    private:
        const string table_name;
        const uint_fast64_t row_count;
    public:
        table_type(const string& table_name, const uint_fast64_t row_count)
            : table_name(table_name), row_count(row_count)
        {

        }

        string get_table_name() const
        {
            return table_name;
        }

        uint_fast64_t get_row_count() const
        {
            return row_count;
        }
    };

    //COLUMN ATTRIBUTES SECTION
    template <typename col_data_type>
    class column_type
    {
    protected:
        const string col_name;
    public:
        column_type(const string& col_name) : col_name(col_name)
        {

        }
        string column_name() const
        {
            return col_name;
        }

        virtual col_data_type operator()() = 0;
        virtual ~column_type<col_data_type>() = default;
    };

    template <typename col_data_type = uint_fast64_t, typename prng_type = uint_fast64_t>
    class normal_int_column : public column_type<col_data_type>
    {
    private:
        prng_engine<prng_type>& prng;
        const table_type& table;
        float cardinality;
        const skewness skew;

        prob_dist_base<col_data_type>* pdfuncbase = nullptr;

        void set_pdf_context()
        {
            if(cardinality == 1)
                pdfuncbase = new simple_incrementer<col_data_type>();
            else if(cardinality < 1)
            {   
                if(skew == skewness::NO)
                    pdfuncbase = new uniform_int_dist_engine<prng_type, col_data_type>(prng, 1,
                        (table.get_row_count()*cardinality));
                else
                    pdfuncbase = new zipf_distribution<prng_type, col_data_type>(prng,
                        table.get_row_count(), get_alpha_value(skew));
            }
            else if(cardinality > 1)
            {
                if(skew == skewness::NO)
                    pdfuncbase = new uniform_int_dist_engine<prng_type, col_data_type>(prng, 1,
                        cardinality);
                else
                    pdfuncbase = new zipf_distribution<prng_type, col_data_type>(prng,
                        cardinality, get_alpha_value(skew));
            }
        }

    public:
        normal_int_column(prng_engine<prng_type>& prng, const table_type& table, 
            const string& col_name, const float cardinality, const skewness skew = skewness::NO) : 
            column_type<col_data_type>(col_name), prng(prng), table(table), 
            cardinality(cardinality), skew(skew)
        {
            set_pdf_context();
        }

        ~normal_int_column()
        {
            delete pdfuncbase;
        }

        inline col_data_type operator()()
        {
            return (*pdfuncbase)();
        }
    };

    template <typename col_data_type = double, typename prng_type = uint_fast64_t>
    class normal_real_column : public column_type<col_data_type>
    {
    private:
        prng_engine<prng_type>& prng;
        const table_type& table;
        float real_min;
        float real_max;
        prob_dist_base<col_data_type>* pdfuncbase = nullptr;
    public:
        normal_real_column(prng_engine<prng_type>& prng, const table_type& table, const string& col_name,
            const float real_min = 0.0, const float real_max = 1.0) : column_type<col_data_type>(col_name),
            prng(prng), table(table), real_min(real_min), real_max(real_max)
        {
            pdfuncbase = new uniform_real_dist_engine<prng_type, col_data_type>(prng, 
                real_min, real_max);
        }

        ~normal_real_column()
        {
            delete pdfuncbase;
        }

        col_data_type operator()()
        {
            return (*pdfuncbase)();
        }

    };

    template <typename col_data_type = string, typename prng_type = uint_fast64_t>
    class normal_string_column : public column_type<string>
    {
    private:
        prng_engine<prng_type>& prng;
        const table_type& table;
        float cardinality;
        uint_fast16_t str_length;
        bool is_var_length;
        const skewness skew;
        prob_dist_base<prng_type>* pdfuncbase = nullptr;
        rand_str_generator<prng_type>* rand_str_gen = nullptr;

        void set_pdf_context()
        {
            if(cardinality == 1)
                pdfuncbase = new simple_incrementer<>();
            else if(cardinality < 1)
            {
                if(skew == skewness::NO)
                    pdfuncbase = new uniform_int_dist_engine<>(prng, 1, 
                        (table.get_row_count()*cardinality));
                else
                    pdfuncbase = new zipf_distribution<>(prng,
                        table.get_row_count(), get_alpha_value(skew));
            }
            else if(cardinality > 1)
            {
                if(skew == skewness::NO)
                    pdfuncbase = new uniform_int_dist_engine<>(prng, 1,
                        cardinality);
                else
                    pdfuncbase = new zipf_distribution<>(prng,
                        cardinality, get_alpha_value(skew));
            }

            if(cardinality >= 0.7 && cardinality <= 1.0)
            {
                bool bypass_warehouse_check = !(is_var_length && skew != skewness::NO);
                rand_str_gen = new rand_str_generator<prng_type>(str_length, is_var_length, 
                    bypass_warehouse_check);
            }
            else if((cardinality > 0 && cardinality < 0.7) || cardinality > 1)
                rand_str_gen = new rand_str_generator<prng_type>(str_length, is_var_length, false);
        }
    public:
        normal_string_column(prng_engine<prng_type>& prng, const table_type& table, const string& col_name,
            const float cardinality, const skewness skew = skewness::NO, const uint_fast16_t str_length = 10, 
            const bool var_length = false) : column_type<col_data_type>(col_name),
            prng(prng), table(table), cardinality(cardinality), skew(skew), str_length(str_length), 
            is_var_length(var_length)
        {
            set_pdf_context();
        }

        ~normal_string_column()
        {
            if(rand_str_gen)
                delete rand_str_gen;
            if(pdfuncbase)
                delete pdfuncbase;
        }

        inline col_data_type operator()()
        {
            string rand_str = "<error_in_string_gen>";
            if(rand_str_gen)
                rand_str = (*rand_str_gen)((*pdfuncbase)());
            return rand_str;
        }
    };

}