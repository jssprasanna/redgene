#include "rglibinc.hpp"
#include "rand_engine.hpp"
#include "prob_dist.hpp"
#include "rg_utils.hpp"
#include "json.hpp"

using namespace redgene;
using json = nlohmann::json;

namespace redgene
{
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

                    if(type == "REAL")
                    {
                        if(constraint != column_obj.end() || cardinality != column_obj.end())
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
}