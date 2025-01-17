#include "rglibinc.hpp"
#include "rand_engine.hpp"
#include "prob_dist.hpp"
#include "rg_utils.hpp"
#include "json.hpp"
#include <type_traits>

using json = nlohmann::json;

namespace redgene
{
    //reference zipf alpha value: NO = NA, LOW = 0.5, MEDIUM = 0.9, HIGH = 1.1, EXTREME = 1.5
    using skewness = enum skewness { NO, LOW, MEDIUM, HIGH, EXTREME };
    using redgene_types = enum redgene_types { INT, REAL, STRING, DATE, TIMESTAMP };
    using constraints = enum cofnstraints { NONE, PK, FK, FK_UNIQUE, COMP_PK, COMP_FK };
 
    static const string UNIX_TIME_EPOCH = "1970-JAN-01 05:30:00";

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
        bool validate_fk_refspec(const string& ref_tab, 
            const string& ref_col, const bool is_composite);
    public:
        redgene_validator() = delete;
        redgene_validator(const string json_filename);
        json& get_redgene_valid_json();
        bool is_valid() const;
        bool is_ref_constraint(json& json_node);
    };

    const set<string> redgene_validator::valid_types = {"INT", "REAL", "STRING", "DATE", "TIMESTAMP"};
    const set<string> redgene_validator::valid_constraints = {"PK", "FK", "COMP_PK", "COMP_FK", "FK_UNIQUE"};
    const set<string> redgene_validator::valid_skewness = {"NO", "LOW", "MEDIUM", "HIGH", "EXTREME"};

    redgene_validator::redgene_validator(const string json_filename)
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

    bool redgene_validator::is_valid() const
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
            
            //bool to check all COMP_PKs are of same profile.
            bool is_comp_pk_a_ref, is_first_occurance = true;
            //map to track all mapping from columns of COMP_FK table to COMP_PK table columns.
            map<string, bool> comp_pk_col_vector;
            for(auto column_obj : column_arr_obj.value())
            {
                auto column_name = column_obj.find("column_name");
                if(column_name == column_obj.end())
                    return false;
                auto col_name_str = column_name.value().get<string>();
                transform(col_name_str.begin(), col_name_str.end(),
                    col_name_str.begin(), ::tolower);
                aux_col_names_set.insert(col_name_str);

                auto col_data_type = column_obj.find("type");
                bool ref_constraint = is_ref_constraint(column_obj);

                //it's invalid to provide both type and reference constraint
                if(col_data_type != column_obj.end() && ref_constraint)
                    return false;

                if(col_data_type == column_obj.end() && !ref_constraint)
                    return false;
                
                if(!ref_constraint)
                {
                    auto col_data_type_str = col_data_type.value().get<string>();
                    transform(col_data_type_str.begin(), col_data_type_str.end(),
                        col_data_type_str.begin(), ::toupper);
                    
                    if(valid_types.find(col_data_type_str) == valid_types.end())
                        return false;
                } 

                //Check for 'CONSTRAINT', 'CARDINALITY' and 'TYPE'
                auto cardinality = column_obj.find("cardinality");
                if(cardinality != column_obj.end())
                {
                    if(cardinality.value().get<float>() <= 0.0)
                        return false;
                }

                auto constraint = column_obj.find("constraint");
                //Logic to check if invalid constraint value is provided.
                if(constraint != column_obj.end())
                {
                    auto constraint_val = constraint.value().get<string>();
                    transform(constraint_val.begin(), constraint_val.end(),
                        constraint_val.begin(), ::toupper);
                    
                    if(valid_constraints.find(constraint_val) == valid_constraints.end())
                        return false;
                    
                    //Logic to check dual nature of COMP_PK column (is either independent or has reference column)
                    if(constraint_val == "COMP_PK")
                    {
                        if(is_first_occurance)
                        {
                            is_comp_pk_a_ref = ref_constraint;
                            is_first_occurance = false;
                        }
                        else
                        {
                            if(is_comp_pk_a_ref != ref_constraint)
                                return false;
                        }
                    }

                    if(constraint_val == "PK" || (constraint_val == "COMP_PK" && !ref_constraint))
                    {
                        //PK constraint column_type should be INT or STRING only.
                        auto col_data_type_str = col_data_type.value().get<string>();
                        transform(col_data_type_str.begin(), col_data_type_str.end(),
                            col_data_type_str.begin(), ::toupper);
                        if(!(col_data_type_str == "INT" || col_data_type_str == "STRING"))
                            return false;
                    }
                    //check whether references are provided for ref constraints
                    //FOREIGN KEY (FK, FK_UNIQUE and COMP_FK)
                    else if(constraint_val == "FK" || constraint_val == "FK_UNIQUE" || 
                        (constraint_val == "COMP_PK" && ref_constraint) || 
                        constraint_val == "COMP_FK")
                    {
                        auto ref_table = column_obj.find("ref_tab");
                        auto ref_column = column_obj.find("ref_col");

                        if((ref_table == column_obj.end()) || (ref_column == column_obj.end()))
                            return false;
                        
                        if(!validate_fk_refspec(ref_table.value().get<string>(),
                            ref_column.value().get<string>(), 
                            (constraint_val == "COMP_FK")))
                            return false;
                        
                        if(constraint_val == "COMP_FK")
                        {
                            //check if comp_pk map is not created yet and create one.
                            //itrerate on comp_pk table and add all comp_pk columns to map.
                            if(comp_pk_col_vector.size() == 0)
                            {
                                for(auto indv_table_obj : tab_arr_obj.value())
                                {
                                    if(indv_table_obj.find("table_name").value().get<string>() == 
                                        ref_table.value().get<string>())
                                    {
                                        for(auto indv_column_obj : indv_table_obj.find("columns").value())
                                        {
                                            if(indv_column_obj.find("constraint") != indv_column_obj.end())
                                                if(indv_column_obj.find("constraint").value() == "COMP_PK")
                                                    comp_pk_col_vector.insert(pair<string, bool>
                                                    (indv_column_obj.find("column_name").value().get<string>(), false));
                                        }
                                    }
                                }
                            }
                            comp_pk_col_vector.find(ref_column.value().get<string>())->second = true;
                        }
                    }
                }
                
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
                            if(str_length.value().get<int_fast16_t>() <= 0 
                                || str_length.value().get<int_fast16_t>() > 4000)
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
                    else if(type == "DATE" || type == "TIMESTAMP")
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

            //see if all mapping of columns from COMP_FK to COMP_PK are present.
            if(comp_pk_col_vector.size() > 0)
                for(map<string, bool>::const_iterator citr = comp_pk_col_vector.cbegin();
                    citr != comp_pk_col_vector.cend(); ++citr)
                       if(!citr->second) return false;

            if(aux_col_names_set.size() != column_arr_obj.value().size())
                return false;
        }

        if(aux_tab_names_set.size() != tab_arr_obj.value().size())
            return false;

        return true;
    }

    json& redgene_validator::get_redgene_valid_json()
    {
        return redgene_json;
    }

    bool redgene_validator::is_ref_constraint(json& json_node)
    {
        bool ref_constraint = false;
        if(json_node.find("constraint") != json_node.end())
        {
            auto constraint = json_node.find("constraint").value().get<string>();
            transform(constraint.begin(), constraint.end(), constraint.begin(), ::toupper);

            if(constraint == "FK" || constraint == "FK_UNIQUE" || constraint == "COMP_FK")
                ref_constraint = true;
            else if(constraint == "COMP_PK")
            {
                ref_constraint = (json_node.find("ref_tab") != json_node.end()) && 
                    (json_node.find("ref_col") != json_node.end());
            }
        }
        return ref_constraint;
    }

    bool redgene_validator::validate_fk_refspec(const string& ref_tab,
        const string& ref_col, const bool is_composite)
    {
        bool fk_ref_valid = false;
        //probe for ref_tab
        auto table_array = redgene_json.find("tables");
        for(auto table : table_array.value())
        {
            if(table.find("table_name").value().get<string>() == ref_tab)
            {
                for(auto column : table.find("columns").value())
                {
                    if(column.find("column_name").value().get<string>() == ref_col)
                    {
                        auto constraint = column.find("constraint");
                        if(constraint != column.end())
                        {
                            if(constraint.value().get<string>() == "PK" && !is_composite)
                                fk_ref_valid = true;
                            else if(constraint.value().get<string>() == "COMP_PK" && is_composite)
                                fk_ref_valid = true;
                        }
                    }
                }
            }
        }
        return fk_ref_valid;
    }


    class column;

    //TABLE ATTRIBUTES SECTION
    class table
    {
    private:
        const string table_name;
        mutable uint_fast64_t row_count;
        unordered_map<string, column*> col_map;
        vector<string> insert_order;
    public:
        table(const string& table_name, const uint_fast64_t row_count)
            : table_name(table_name), row_count(row_count)
        {

        }

        void insert_column_metadata_obj(string& column_name, column* col_metadata)
        {
            col_map.insert(pair<string, column*>(column_name, col_metadata));
            insert_order.push_back(column_name);
        }

        unordered_map<string, column*>& get_column_map()
        {
            return col_map;
        }

        vector<string> get_column_order()
        {
            return insert_order;
        }

        string get_table_name() const
        {
            return table_name;
        }

        uint_fast64_t get_row_count() const
        {
            return row_count;
        }

        void set_row_count(uint_fast64_t row_count)
        {
            this->row_count = row_count;
        }
    };

    //COLUMN ATTRIBUTES SECTION
    class column
    {
    protected:
        const string col_name;
        mutable redgene_types data_type;
        const constraints constraint;
    public:
        column(const string& col_name, const redgene_types data_type,
            const constraints constraint) : 
            col_name(col_name), data_type(data_type), constraint(constraint)
        {

        }
        string column_name() const
        {
            return col_name;
        }

        redgene_types get_type() const
        {
            return data_type;
        }

        constraints get_constraint_type() const
        {
            return constraint;
        }

        void set_data_type(redgene_types data_type)
        {
            this->data_type = data_type;
        }

        virtual ~column() = default;
    };

    //SECTION-1: NORMAL COLUMN GENERATION CLASSES
    class normal_int_column : public column
    {
    private:
        prng_engine<uint_fast64_t>& prng;
        const table& _table;
        float cardinality;
        const skewness skew;

        prob_dist_base<uint_fast64_t>* pdfuncbase = nullptr;

        //Members to support cardinality specification between 0 and 1
        prob_dist_base<uint_fast64_t>* support_pdf_lt1card = nullptr;
        uint_fast64_t call_index = 0;

        void set_pdf_context()
        {
            if(cardinality == 1)
                pdfuncbase = new simple_incrementer<uint_fast64_t>();
            else if(cardinality < 1)
            {   
                call_index = _table.get_row_count()*cardinality;
                support_pdf_lt1card = new simple_incrementer<uint_fast64_t>();

                if(skew == skewness::NO)
                    pdfuncbase = new uniform_int_dist_engine<uint_fast64_t, uint_fast64_t>(prng, 1,
                        (_table.get_row_count()*cardinality));
                else
                    pdfuncbase = new zipf_distribution<uint_fast64_t, uint_fast64_t>(prng,
                        _table.get_row_count()*cardinality, get_alpha_value(skew));
            }
            else if(cardinality > 1)
            {
                if(skew == skewness::NO)
                    pdfuncbase = new uniform_int_dist_engine<uint_fast64_t, uint_fast64_t>(prng, 1,
                        cardinality);
                else
                    pdfuncbase = new zipf_distribution<uint_fast64_t, uint_fast64_t>(prng,
                        cardinality, get_alpha_value(skew));
            }
        }

    public:
        normal_int_column(prng_engine<uint_fast64_t>& prng, const table& table, 
            const string& col_name, const float cardinality, const skewness skew = skewness::NO) : 
            column(col_name, redgene_types::INT, constraints::NONE), prng(prng), _table(table), 
            cardinality(cardinality), skew(skew)
        {
            set_pdf_context();
        }

        ~normal_int_column()
        {
            delete pdfuncbase;
            if(support_pdf_lt1card)
                delete support_pdf_lt1card;
        }

        virtual uint_fast64_t yield()
        {
            uint_fast64_t ret_val;
            if(cardinality < 1)
            {
                if(call_index > 0)
                {
                    ret_val = (*support_pdf_lt1card)();
                    --call_index;
                }
                else
                    ret_val = (*pdfuncbase)();
            }
            else
                ret_val = (*pdfuncbase)();

            return ret_val;
        }
    };

    class normal_real_column : public column
    {
    private:
        prng_engine<uint_fast64_t>& prng;
        const table& _table;
        float real_min;
        float real_max;
        prob_dist_base<double>* pdfuncbase = nullptr;
    public:
        normal_real_column(prng_engine<uint_fast64_t>& prng, const table& table, const string& col_name,
            const float real_min = 0.0, const float real_max = 1.0) : column(col_name, redgene_types::REAL,
            constraints::NONE), prng(prng), _table(table), real_min(real_min), real_max(real_max)
        {
            pdfuncbase = new uniform_real_dist_engine<uint_fast64_t, double>(prng, 
                real_min, real_max);
        }

        ~normal_real_column()
        {
            delete pdfuncbase;
        }

        inline double yield()
        {
            return (*pdfuncbase)();
        }

    };

    class normal_date_column : public column
    {
    private:
        prng_engine<uint_fast64_t>& prng;
        const table& _table;
        const string user_date;
        uint_fast8_t range_in_years;
        prob_dist_base<uint_fast64_t>* pdfuncbase = nullptr;

        chrono::system_clock::time_point ref_timepoint;
        chrono::system_clock::time_point adv_timepoint;
        const uint64_t sec_a_year = 31536000;
        chrono::seconds interval_in_second;

        //objects to map user-defined date time reference
        time_t rawtime, time_tobj;
        struct tm *tm_map, *ltm;
        string* date_string = nullptr;
        string* mon_str = nullptr;

        const string& get_month(int mon_no)
        {
            switch(mon_no)
            {
                case 0: *mon_str = "JAN"; break;
                case 1: *mon_str = "FEB"; break;
                case 2: *mon_str = "MAR"; break;
                case 3: *mon_str = "APR"; break;
                case 4: *mon_str = "MAY"; break;
                case 5: *mon_str = "JUN"; break;
                case 6: *mon_str = "JUL"; break;
                case 7: *mon_str = "AUG"; break;
                case 8: *mon_str = "SEP"; break;
                case 9: *mon_str = "OCT"; break;
                case 10: *mon_str = "NOV"; break;
                case 11: *mon_str = "DEC"; break;
            }
            return *mon_str;
        }

    public:
        normal_date_column(prng_engine<uint_fast64_t>& p_prng, 
            const table& p_table, const string& p_col_name, 
            const string p_user_date = redgene::UNIX_TIME_EPOCH, 
            const uint_fast8_t p_range_in_years = 10) :
            column(p_col_name, redgene_types::DATE, constraints::NONE),
            prng(p_prng), _table(p_table), user_date(p_user_date), 
            range_in_years(p_range_in_years)
        {
            if(user_date != redgene::UNIX_TIME_EPOCH)
            {
                time(&rawtime);
                tm_map = localtime(&rawtime);
                istringstream user_date_str(user_date);
                user_date_str >> get_time(tm_map, "%Y-%b-%d");
                rawtime =  mktime(tm_map);

                ref_timepoint = chrono::system_clock::from_time_t(rawtime);
            }
            adv_timepoint = ref_timepoint + 
                chrono::seconds(sec_a_year*range_in_years);

            interval_in_second = chrono::duration_cast<chrono::seconds>
                (adv_timepoint - ref_timepoint);
            
            pdfuncbase = new uniform_int_dist_engine<>(prng, 
                0, interval_in_second.count());
            date_string = new string(11, ' ');
            mon_str = new string(3, ' ');
        }

        const string& yield()
        {
            time_tobj = std::chrono::system_clock::to_time_t(
                ref_timepoint + chrono::seconds((*pdfuncbase)()));
            ltm = localtime(&time_tobj);            
            
            stringstream ss;
            ss << setfill('0') << setw(2) << ltm->tm_mday 
                << "-" << get_month(ltm->tm_mon)
                << "-" << 1900+ltm->tm_year;

            *date_string = ss.str();
            return *date_string;
        }

        ~normal_date_column()
        {
            if(pdfuncbase)
                delete pdfuncbase;
            if(date_string)
                delete date_string;
            if(mon_str)
                delete mon_str;
        }
    };

    class normal_timestamp_column : public column
    {
    private:
        prng_engine<uint_fast64_t>& prng;
        const table& _table;
        const string user_date;
        uint_fast8_t range_in_years;
        prob_dist_base<uint_fast64_t>* pdfuncbase = nullptr;

        chrono::system_clock::time_point ref_timepoint;
        chrono::system_clock::time_point adv_timepoint;
        const uint64_t sec_a_year = 31536000;
        chrono::seconds interval_in_second;

        //objects to map user-defined date time reference
        time_t rawtime, time_tobj;
        struct tm *tm_map, *ltm;
        string* date_string = nullptr;
        string* mon_str = nullptr;

        const string& get_month(int mon_no)
        {
            switch(mon_no)
            {
                case 0: *mon_str = "JAN"; break;
                case 1: *mon_str = "FEB"; break;
                case 2: *mon_str = "MAR"; break;
                case 3: *mon_str = "APR"; break;
                case 4: *mon_str = "MAY"; break;
                case 5: *mon_str = "JUN"; break;
                case 6: *mon_str = "JUL"; break;
                case 7: *mon_str = "AUG"; break;
                case 8: *mon_str = "SEP"; break;
                case 9: *mon_str = "OCT"; break;
                case 10: *mon_str = "NOV"; break;
                case 11: *mon_str = "DEC"; break;
            }
            return *mon_str;
        }

    public:
        normal_timestamp_column(prng_engine<uint_fast64_t>& p_prng, 
            const table& p_table, const string& p_col_name, 
            const string p_user_date = redgene::UNIX_TIME_EPOCH, 
            const uint_fast8_t p_range_in_years = 10) :
            column(p_col_name, redgene_types::TIMESTAMP, constraints::NONE),
            prng(p_prng), _table(p_table), user_date(p_user_date), 
            range_in_years(p_range_in_years)
        {
            if(user_date != redgene::UNIX_TIME_EPOCH)
            {
                time(&rawtime);
                tm_map = localtime(&rawtime);
                istringstream user_date_str(user_date);
                user_date_str >> get_time(tm_map, "%Y-%b-%d %H:%M:%S");
                rawtime =  mktime(tm_map);

                ref_timepoint = chrono::system_clock::from_time_t(rawtime);
            }
            adv_timepoint = ref_timepoint + 
                chrono::seconds(sec_a_year*range_in_years);

            interval_in_second = chrono::duration_cast<chrono::seconds>
                (adv_timepoint - ref_timepoint);
            
            pdfuncbase = new uniform_int_dist_engine<>(prng, 
                0, interval_in_second.count());
            date_string = new string(25, ' ');
            mon_str = new string(3, ' ');
        }

        const string& yield()
        {
            time_tobj = std::chrono::system_clock::to_time_t(
                ref_timepoint + chrono::seconds((*pdfuncbase)()));
            ltm = localtime(&time_tobj);            
            
            stringstream ss;
            ss << setfill('0') << setw(2) << ltm->tm_mday 
                << "-" << get_month(ltm->tm_mon)
                << "-" << 1900+ltm->tm_year << " "
                << setw(2) << ltm->tm_hour << ":"
                << setw(2) << ltm->tm_min << ":"
                << setw(2) << ltm->tm_sec;

            *date_string = ss.str();
            return *date_string;
        }

        ~normal_timestamp_column()
        {
            if(pdfuncbase)
                delete pdfuncbase;
            if(date_string)
                delete date_string;
            if(mon_str)
                delete mon_str;
        }
    };
    
    class normal_string_column : public column
    {
    private:
        prng_engine<uint_fast64_t>& prng;
        const table& _table;
        float cardinality;
        uint_fast16_t str_length;
        bool is_var_length;
        const skewness skew;
        prob_dist_base<uint_fast64_t>* pdfuncbase = nullptr;
        rand_str_generator<>* rand_str_gen = nullptr;

        //Members to support cardinality specification between 0 and 1
        prob_dist_base<uint_fast64_t>* support_pdf_lt1card = nullptr;
        uint_fast64_t call_index = 0;

        void set_pdf_context()
        {
            if(cardinality == 1)
                pdfuncbase = new simple_incrementer<>();
            else if(cardinality < 1)
            {
                call_index = _table.get_row_count()*cardinality;
                support_pdf_lt1card = new simple_incrementer<uint_fast64_t>();

                if(skew == skewness::NO)
                    pdfuncbase = new uniform_int_dist_engine<>(prng, 1, 
                        (_table.get_row_count()*cardinality));
                else
                    pdfuncbase = new zipf_distribution<>(prng,
                        _table.get_row_count()*cardinality, get_alpha_value(skew));
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
                rand_str_gen = new rand_str_generator<>(str_length, is_var_length, 
                    bypass_warehouse_check);
            }
            else if((cardinality > 0 && cardinality < 0.7) || cardinality > 1)
                rand_str_gen = new rand_str_generator<>(str_length, is_var_length, false);
        }
    public:
        normal_string_column(prng_engine<uint_fast64_t>& prng, const table& table, const string& col_name,
            const float cardinality, const skewness skew = skewness::NO, const uint_fast16_t str_length = 10, 
            const bool var_length = false) : column(col_name, redgene_types::STRING, constraints::NONE),
            prng(prng), _table(table), cardinality(cardinality), skew(skew), str_length(str_length), 
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
            if(support_pdf_lt1card)
                delete support_pdf_lt1card;
        }

        virtual const string& yield()
        {
            uint_fast64_t key_val;
            if(cardinality < 1)
            {
                if(call_index > 0)
                {
                    key_val = (*support_pdf_lt1card)();
                    --call_index;
                }
                else
                    key_val = (*pdfuncbase)();
            }
            else
                key_val = (*pdfuncbase)();
            return (*rand_str_gen)(key_val);
        }

        uint_fast16_t get_str_length() const
        {
            return str_length;
        }

        bool get_is_var_length() const
        {
            return is_var_length;
        }
    };

    class pk_int_column : public normal_int_column
    {
    public:
        pk_int_column(prng_engine<uint_fast64_t>& prng, const table& table,
            const string& col_name) : normal_int_column(prng, table, col_name, 1)
        {

        }
        
        ~pk_int_column() = default;

        virtual inline uint_fast64_t yield()
        {
            return this->normal_int_column::yield();
        }
    };

    class pk_string_column : public normal_string_column
    {
    public:
        pk_string_column(prng_engine<uint_fast64_t>& prng, const table& table,
            const string& col_name, const uint_fast16_t str_length = 10, 
            const bool var_length = false) : normal_string_column(prng, table, col_name, 
            1, skewness::NO, str_length, var_length)
        {

        }

        ~pk_string_column() = default;

        virtual const string& yield()
        {
            return normal_string_column::yield();
        }
    };
    
    class fk_int_column : public normal_int_column
    {
    private:
        float fk_cardinality;
    public:
        fk_int_column(prng_engine<uint_fast64_t>& prng, const table& table,
            const string& col_name, const float cardinality, const skewness skew = skewness::NO)
            : normal_int_column(prng, table, col_name, cardinality, skew), fk_cardinality(cardinality)
        {

        }

        ~fk_int_column() = default;

        virtual inline uint_fast64_t yield()
        {
            return normal_int_column::yield();
        }
    };

    class fk_string_column : public normal_string_column
    {
    private:
        float cardinality;
        uint_fast16_t str_length;
        bool var_length;
    public:
        fk_string_column(prng_engine<uint_fast64_t>& prng, const table& table,
            const string& col_name, const float cardinality, const uint_fast16_t str_length, 
            const bool var_length, const skewness skew = skewness::NO) :
            normal_string_column(prng, table, col_name, cardinality, skew, str_length, var_length)
        {

        }

        ~fk_string_column() = default;

        virtual const string& yield()
        {
            return normal_string_column::yield();
        }
    };

    class fk_unique_int_column : public normal_int_column
    {
    public:
        fk_unique_int_column(prng_engine<uint_fast64_t>& prng, const table& table, 
            const string& col_name) :
            normal_int_column(prng, table, col_name, 1)
        {

        }

        ~fk_unique_int_column() = default;

        inline uint_fast64_t yield()
        {
            return normal_int_column::yield();
        }
    };

    class fk_unique_string_column : public normal_string_column
    {
    public:
        fk_unique_string_column(prng_engine<uint_fast64_t>& prng, const table& table,
            const string& col_name, const uint_fast16_t str_length, const bool var_length) :
            normal_string_column(prng, table, col_name, 1, skewness::NO, str_length, var_length)
        {

        }

        ~fk_unique_string_column() = default;

        const string& yield()
        {
            return normal_string_column::yield();
        }
    };

    //int composite primary key class
    class comp_pk_int_column : public column
    {
    private:
        prng_engine<uint_fast64_t>& prng;
        const table& _table;
        
        const uint_fast64_t amount;
        const uint_fast64_t max;
        const uint_fast64_t repeat_window;
        const uint_fast64_t group_size;

        prob_dist_base<uint_fast64_t>* pdfuncbase = nullptr;
    public:
        comp_pk_int_column(prng_engine<uint_fast64_t>& prng, const table& _table, const string& col_name,
            const uint_fast64_t prng_seed, const uint_fast64_t amount, const uint_fast64_t max, 
            const uint_fast64_t repeat_window, const uint_fast64_t group_size) :
            prng(prng), _table(_table), column(col_name, redgene_types::INT, constraints::COMP_PK), 
            amount(amount), max(max), repeat_window(repeat_window), group_size(group_size)
        {
            //reset prng to seed value provided.
            //so that we can have all columns invovled in the comp_pk generation 
            //be having same random vector to generate the combination of comp_pk values 
            prng.seed(prng_seed);
            pdfuncbase = new set_distribution<uint_fast64_t>(prng, amount, max);
        }

        ~comp_pk_int_column()
        {
            if(pdfuncbase)
                delete pdfuncbase;
        }

        uint_fast64_t get_distinct_count() const
        {
            return amount;
        }

        uint_fast64_t get_maxvalue() const
        {
            return max;
        }

        uint_fast64_t get_repeat_window() const
        {
            return repeat_window;
        }

        uint_fast64_t get_group_size() const
        {
            return group_size;
        }

        inline uint_fast64_t yield()
        {
            uint_fast64_t tmp = (*pdfuncbase)();
            tmp %= repeat_window;
            if(tmp == 0)
                tmp = repeat_window;
            return ceil((double)tmp / group_size);
        }
    };

    class comp_pk_string_column : public comp_pk_int_column
    {
    private:
         uint_fast16_t string_length;
         bool var_length;
         rand_str_generator<>* rand_str_gen = nullptr;
    public:
        comp_pk_string_column(prng_engine<uint_fast64_t>& prng, const table& table, const string& col_name, 
            const uint_fast64_t prng_seed, const uint_fast64_t amount, const uint_fast64_t max, 
            const uint_fast64_t repeat_window, const uint_fast64_t group_size, const uint_fast16_t str_length,
            const bool var_length) :
            comp_pk_int_column(prng, table, col_name, prng_seed, amount, max, repeat_window, group_size), 
            string_length(str_length), var_length(var_length)
        {
            comp_pk_int_column::column::set_data_type(redgene_types::STRING);
            rand_str_gen = new rand_str_generator<>(str_length, var_length, !var_length);
        }

        ~comp_pk_string_column()
        {
            if(rand_str_gen)
                delete rand_str_gen;
        }

        uint_fast16_t get_string_length() const
        {
            return string_length;
        }

        bool get_is_var_length() const
        {
            return var_length;
        }

        const string& yield()
        {
            return (*rand_str_gen)(comp_pk_int_column::yield());
        }
    };

    //COMP_FK classes
    class comp_fk_int_column : public column
    {
    private:
        prng_engine<uint_fast64_t> prng;
        const table& _table;
        const uint_fast64_t repeat_window;
        const uint_fast64_t group_size;
        const float cardinality;
        const skewness skew;
        vector<uint_fast64_t>* comp_pk_set_dist_vector = nullptr;
        prob_dist_base<uint_fast64_t>* pdfuncbase = nullptr;
    public:
        comp_fk_int_column(prng_engine<uint_fast64_t>& _prng, const table& _table, const string& col_name,
            const uint_fast64_t prng_seed, const uint_fast64_t amount, const uint_fast64_t max, 
            const uint_fast64_t repeat_window, const uint_fast64_t group_size, const float cardinality,
            const skewness skew) : prng(_prng), _table(_table), column(col_name, redgene_types::INT,
            constraints::COMP_FK), repeat_window(repeat_window), group_size(group_size), 
            cardinality(cardinality), skew(skew), comp_pk_set_dist_vector(new vector<uint_fast64_t>(amount))
        {
            //repopulate all COMP_PK set distribution vector
            prng.seed(prng_seed);
            set_distribution<uint_fast64_t> set_dist_pdf(prng, amount, max);
            for(uint_fast64_t i = 0; i < amount; ++i)
                (*comp_pk_set_dist_vector)[i] = set_dist_pdf();

            if(skew == skewness::NO)
            {
                pdfuncbase = new uniform_int_dist_engine<>(prng, 1, cardinality);
            }
            else
                pdfuncbase = new zipf_distribution<>(prng, cardinality, get_alpha_value(skew));
        }

        ~comp_fk_int_column()
        {
            if(comp_pk_set_dist_vector)
                delete comp_pk_set_dist_vector;
            if(pdfuncbase)
                delete pdfuncbase;
        }

        inline uint_fast64_t yield()
        {   
            uint_fast64_t index = (*pdfuncbase)()-1;
            uint_fast64_t set_vector_val = (*comp_pk_set_dist_vector)[index];
            set_vector_val %= repeat_window;
            if(set_vector_val == 0)
                set_vector_val = repeat_window;
            return ceil((double)set_vector_val / group_size);
        }
    };

    class comp_fk_string_column :public comp_fk_int_column
    {
    private:
        uint_fast16_t string_length;
        bool var_length;
        rand_str_generator<>* rand_str_gen = nullptr;
    public:
        comp_fk_string_column(prng_engine<uint_fast64_t>& _prng, const table& _table, const string& col_name,
            const uint_fast64_t prng_seed, const uint_fast64_t amount, const uint_fast64_t max, 
            const uint_fast64_t repeat_window, const uint_fast64_t group_size, const float cardinality,
            const skewness skew, const uint_fast16_t str_length, const bool var_length) :
            comp_fk_int_column(_prng, _table, col_name, prng_seed, amount, max, repeat_window, group_size,
            cardinality, skew), string_length(str_length), var_length(var_length)
        {
            comp_fk_int_column::column::set_data_type(redgene_types::STRING);
            //since this is FK column we will not bypass string warehouse check.
            rand_str_gen = new rand_str_generator<>(string_length, var_length, false);
        }

        ~comp_fk_string_column()
        {
            if(rand_str_gen)
                delete rand_str_gen;
        }

        const string& yield()
        {
            return (*rand_str_gen)(comp_fk_int_column::yield());
        }
    };


    class redgene_engine
    {
    private:    
        redgene_validator& rgene_validator;
        prng_engine<uint_fast64_t>* prng = nullptr;
        map<string, table*> schema_map;
        uint_fast64_t g_prng_seed;

        //members to track comp_pk columns
        typedef struct comp_pk_attributes
        {
            uint_fast64_t repeat_window;
            uint_fast64_t group_size;
        } comp_pk_attributes;
        bool is_comp_pk_map_available = false;
        map<string, comp_pk_attributes*>* comp_pk_attrib_map = nullptr;
    public:
        redgene_engine() = delete;
        redgene_engine(redgene_validator& rgene_validator) :
            rgene_validator(rgene_validator)
        {

        }
        ~redgene_engine()
        {
            if(prng)
                delete prng;

            for(auto table : schema_map)
            {
                for(auto column : table.second->get_column_map())
                {
                    delete column.second;
                }
                delete table.second;
            }
        }
        void generate()
        {
            generate_redgene_structures();
        }
    private:
        void generate_redgene_structures()
        {
            if(rgene_validator.is_valid())
            {
                set_prng_engine();
                set_table_metadata();
                datagen();
            }
            else
            {
                throw runtime_error("json file is not ReDGene valid!");
            }
        }

        void set_prng_engine()
        {
            json& rgene_json = rgene_validator.get_redgene_valid_json();
            auto prng_seed = (rgene_json.find("seed") != rgene_json.end()) ?
                rgene_json.find("seed").value().get<uint_fast64_t>() : 1729;
            
            string prng_type = (rgene_json.find("prng") != rgene_json.end()) ? 
                rgene_json.find("prng").value().get<string>() : "DEFAULT";
            random_engines prng_engine_type;

            if(prng_type == "MINSTD_RAND0")
                prng_engine_type = random_engines::MINSTD_RAND0;
            else if(prng_type == "MINSTD_RAND1")
                prng_engine_type = random_engines::MINSTD_RAND1;
            else if(prng_type == "MT19937")
                prng_engine_type = random_engines::MT19937;
            else if(prng_type == "MT19937_64")
                prng_engine_type = random_engines::MT19937_64;
            else if(prng_type == "RANLUX24")
                prng_engine_type = random_engines::RANLUX24;
            else if(prng_type == "RANLUX48")
                prng_engine_type = random_engines::RANLUX48;

            g_prng_seed = prng_seed;
            prng = new prng_engine<uint_fast64_t>(prng_engine_type, prng_seed);
        }

        uint_fast64_t create_nonref_comp_pk_map(json& column_arr, uint_fast64_t row_count)
        {
            comp_pk_attrib_map =  new map<string, comp_pk_attributes*>();
            for(auto column : column_arr)
            {
                if(column.find("constraint") != column.end())
                {
                    if(get_redgene_constraint(column.find("constraint").value().get<string>())
                        == constraints::COMP_PK)
                    {
                        comp_pk_attrib_map->insert(pair<string, comp_pk_attributes*>
                            (column.find("column_name").value().get<string>(), new comp_pk_attributes));
                    }
                }
            }

            //calculate alpha value = pow(4*row_count, 1.0/comp_pk_attrib_map.size())
            uint_fast64_t alpha = ceil(pow(4*row_count, 1.0/comp_pk_attrib_map->size()));

            uint_fast16_t index = 0;
            for(auto column : column_arr)
            {
                if(column.find("constraint") != column.end())
                {
                    if(get_redgene_constraint(column.find("constraint").value().get<string>())
                        == COMP_PK)
                    {
                        auto comp_pk_attrib_item = comp_pk_attrib_map->find(
                            column.find("column_name").value().get<string>())->second;
                        comp_pk_attrib_item->repeat_window = pow(alpha, index + 1);
                        comp_pk_attrib_item->group_size = pow(alpha, index);
                        ++index;
                    }
                }
            }
            is_comp_pk_map_available = true;
            return pow(alpha, comp_pk_attrib_map->size());
        }

        uint_fast64_t create_ref_comp_pk_map(json& column_arr)
        {
            uint_fast64_t tmp_group_size = 1;
            comp_pk_attrib_map = new map<string, comp_pk_attributes*>();
            for(auto column : column_arr)
            {
                if(column.find("constraint") != column.end())
                {
                    if(get_redgene_constraint(column.find("constraint").value().get<string>())
                        == constraints::COMP_PK)
                    {
                        comp_pk_attributes* comp_pk_attrib = new comp_pk_attributes;
                        comp_pk_attrib->repeat_window = schema_map.find(column.find("ref_tab").value().get<string>())
                            ->second->get_row_count() * tmp_group_size;
                        comp_pk_attrib->group_size = tmp_group_size;
                        tmp_group_size = comp_pk_attrib->repeat_window;
                        comp_pk_attrib_map->insert(pair<string, comp_pk_attributes*>(column.find("column_name").value().get<string>(),
                            comp_pk_attrib));
                    }
                }
            }
            is_comp_pk_map_available = true;
            return tmp_group_size;
        }

        void set_table_metadata()
        {
            json& rgene_json = rgene_validator.get_redgene_valid_json();
            auto table_arr = rgene_json.find("tables");

            //individual table metadata handling
            for(auto table_obj : table_arr.value())
            {
                auto table_name = table_obj.find("table_name").value().get<string>();
                auto row_count = table_obj.find("row_count").value().get<uint_fast64_t>();

                //table object
                table* table_metadata_obj = new table(table_name, row_count);

                //logic for individual column types
                auto column_arr = table_obj.find("columns");
                uint_fast64_t max_combinations;
                for(auto column_obj : column_arr.value())
                {
                    auto column_name = column_obj.find("column_name").value().get<string>();
                    auto constraint_obj = column_obj.find("constraint");
                    redgene_types column_type;
                    
                    if(constraint_obj == column_obj.end() || !(rgene_validator.is_ref_constraint(column_obj)))
                        column_type = get_redgene_type(column_obj.find("type").value().get<string>());
                    else
                    {
                        column_type = schema_map.find(
                            column_obj.find("ref_tab").value().get<string>())->second->get_column_map().
                            find(column_obj.find("ref_col").value().get<string>())->second->get_type();
                    }

                    //column object
                    column* column_metadata_obj;

                    if(column_type == redgene_types::INT)
                    { 
                        constraints constraint;
                        float cardinality;
                        skewness skew = skewness::NO;
                        if(column_obj.find("skewness") != column_obj.end())
                            skew = get_skewness_type(column_obj.find("skewness").value().get<string>());
                        
                        if(constraint_obj != column_obj.end())
                        {
                            constraint = get_redgene_constraint(constraint_obj.value().get<string>());
                            if(constraint == constraints::PK)
                                column_metadata_obj = new pk_int_column(*prng, *table_metadata_obj, column_name);
                            else if(constraint == constraints::FK)
                            {
                                //cardinality needs to be computed, from ref_tab.ref_col
                                float cardinality = schema_map.find(
                                    column_obj.find("ref_tab").value().get<string>())->second->get_row_count();
                
                                column_metadata_obj = new fk_int_column(*prng, *table_metadata_obj, column_name,
                                    cardinality, skew);
                            }
                            else if(constraint == constraints::FK_UNIQUE)
                            {
                                float cardinality = schema_map.find(
                                    column_obj.find("ref_tab").value().get<string>())->second->get_row_count();
                                
                                if(row_count >= cardinality)
                                {
                                    row_count = cardinality;
                                    table_metadata_obj->set_row_count(row_count);
                                }
                                column_metadata_obj = new fk_unique_int_column(*prng, *table_metadata_obj, column_name);
                            }
                            else if(constraint == constraints::COMP_PK)
                            {
                                if(!is_comp_pk_map_available)
                                {
                                    if(column_obj.find("type") != column_obj.end())
                                        max_combinations = create_nonref_comp_pk_map(column_arr.value(), row_count);
                                    else
                                        max_combinations = create_ref_comp_pk_map(column_arr.value());
                                }
                                auto comp_pk_metadata_obj = comp_pk_attrib_map->find(column_name)->second;
                                column_metadata_obj = new comp_pk_int_column(*prng, *table_metadata_obj, column_name,
                                    g_prng_seed, row_count, max_combinations, comp_pk_metadata_obj->repeat_window,
                                    comp_pk_metadata_obj->group_size);
                            }
                            else if(constraint == constraints::COMP_FK)
                            {
                                auto comp_pk_tab_metadata = schema_map.find(column_obj.find("ref_tab").value().get<string>())->second;
                                auto comp_pk_col_metadata = dynamic_cast<comp_pk_int_column*>(comp_pk_tab_metadata->get_column_map()
                                    .find(column_obj.find("ref_col").value().get<string>())->second);

                                column_metadata_obj = new comp_fk_int_column(*prng, *table_metadata_obj, column_name,
                                    g_prng_seed, comp_pk_tab_metadata->get_row_count(), comp_pk_col_metadata->get_maxvalue(),
                                    comp_pk_col_metadata->get_repeat_window(), comp_pk_col_metadata->get_group_size(),
                                    comp_pk_tab_metadata->get_row_count(), skew);
                            }
                        }
                        else
                        {
                            if(column_obj.find("cardinality") != column_obj.end())
                                cardinality = column_obj.find("cardinality").value().get<float>();

                            column_metadata_obj = new normal_int_column(*prng, *table_metadata_obj,
                                column_name, cardinality, skew);
                        }
                    }
                    else if(column_type == redgene_types::STRING)
                    {
                        constraints constraint;
                        uint_fast16_t string_length = 10;
                        bool var_length = false;
                        float cardinality;
                        skewness skew = skewness::NO;

                        if(column_obj.find("length") != column_obj.end())
                            string_length = column_obj.find("length").value().get<uint_fast16_t>();

                        if(column_obj.find("var_length") != column_obj.end())
                            var_length = column_obj.find("var_length").value().get<bool>();

                        if(column_obj.find("cardinality") != column_obj.end())
                            cardinality = column_obj.find("cardinality").value().get<float>();
                        
                        if(column_obj.find("skewness") != column_obj.end())
                            skew = get_skewness_type(column_obj.find("skewness").value().get<string>());

                        if(constraint_obj != column_obj.end())
                        {
                            constraint = get_redgene_constraint(constraint_obj.value().get<string>());
                            if(constraint == constraints::PK)
                                column_metadata_obj = new pk_string_column(*prng, *table_metadata_obj, column_name,
                                    string_length, var_length);
                            else if(constraint == constraints::FK)
                            {
                                //cardinality needs to be computed, from ref_tab.ref_col
                                cardinality = schema_map.find(
                                    column_obj.find("ref_tab").value().get<string>()
                                    )->second->get_row_count();
                                
                                //str_length needs to be obtained from ref_tab.ref_col
                                auto ref_col_obj_ref = dynamic_cast<pk_string_column*>((schema_map.find(
                                    column_obj.find("ref_tab").value().get<string>()
                                    )->second->get_column_map()).find(column_obj.find("ref_col").value().get<string>())->second);
                                
                                string_length = ref_col_obj_ref->get_str_length();
                                //var_length needs to be obtained from ref_tab.ref_col
                                var_length = ref_col_obj_ref->get_is_var_length();

                                column_metadata_obj = new fk_string_column(*prng, *table_metadata_obj, column_name,
                                    cardinality, string_length, var_length, skew);
                            }
                            else if(constraint == constraints::FK_UNIQUE)
                            {
                                //cardinality needs to be computed, from ref_tab.ref_col
                                cardinality = schema_map.find(
                                    column_obj.find("ref_tab").value().get<string>())->second->get_row_count();
                                
                                //str_length needs to be obtained from ref_tab.ref_col
                                auto ref_col_obj_ref = dynamic_cast<pk_string_column*>((schema_map.find(
                                    column_obj.find("ref_tab").value().get<string>()
                                    )->second->get_column_map()).find(column_obj.find("ref_col").value().get<string>())->second);
                                
                                string_length = ref_col_obj_ref->get_str_length();
                                //var_length needs to be obtained from ref_tab.ref_col
                                var_length = ref_col_obj_ref->get_is_var_length();

                                if(row_count >= cardinality)
                                {
                                    row_count = cardinality;
                                    table_metadata_obj->set_row_count(row_count);
                                }
                                column_metadata_obj = new fk_unique_string_column(*prng, *table_metadata_obj,
                                        column_name, string_length, var_length);
                            }
                            else if(constraint == constraints::COMP_PK)
                            {
                                if(!is_comp_pk_map_available)
                                {
                                    if(column_obj.find("type") != column_obj.end())
                                        max_combinations = create_nonref_comp_pk_map(column_arr.value(), row_count);
                                    else
                                        max_combinations = create_ref_comp_pk_map(column_arr.value());
                                }
                                if(column_obj.find("type") == column_obj.end())
                                {
                                    //str_length needs to be obtained from ref_tab.ref_col
                                    auto ref_col_obj_ref = dynamic_cast<pk_string_column*>((schema_map.find(
                                        column_obj.find("ref_tab").value().get<string>()
                                        )->second->get_column_map()).find(column_obj.find("ref_col").value().get<string>())->second);
                                    
                                    string_length = ref_col_obj_ref->get_str_length();
                                    //var_length needs to be obtained from ref_tab.ref_col
                                    var_length = ref_col_obj_ref->get_is_var_length();
                                }
                                else
                                {
                                    if(column_obj.find("length") != column_obj.end())
                                        string_length = column_obj.find("length").value().get<uint_fast16_t>();
                                    if(column_obj.find("var_length") != column_obj.end())
                                        var_length = column_obj.find("var_length").value().get<bool>();
                                }

                                auto comp_pk_metadata_obj = comp_pk_attrib_map->find(column_name)->second;
                                column_metadata_obj = new comp_pk_string_column(*prng, *table_metadata_obj, column_name,
                                    g_prng_seed, row_count, max_combinations, comp_pk_metadata_obj->repeat_window,
                                    comp_pk_metadata_obj->group_size, string_length, var_length);
                            }
                            else if(constraint == constraints::COMP_FK)
                            {
                                auto comp_pk_tab_metadata = schema_map.find(column_obj.find("ref_tab").value().get<string>())->second;
                                auto comp_pk_col_metadata = dynamic_cast<comp_pk_string_column*>(comp_pk_tab_metadata->get_column_map()
                                    .find(column_obj.find("ref_col").value().get<string>())->second);

                                column_metadata_obj = new comp_fk_string_column(*prng, *table_metadata_obj, column_name,
                                    g_prng_seed, comp_pk_tab_metadata->get_row_count(), comp_pk_col_metadata->get_maxvalue(),
                                    comp_pk_col_metadata->get_repeat_window(), comp_pk_col_metadata->get_group_size(),
                                    comp_pk_tab_metadata->get_row_count(), skew, comp_pk_col_metadata->get_string_length(),
                                    comp_pk_col_metadata->get_is_var_length());
                            }
                        }
                        else
                        {
                            column_metadata_obj = new normal_string_column(*prng, *table_metadata_obj, 
                                column_name, cardinality, skew, string_length, var_length);
                        }
                    }
                    else if(column_type == redgene_types::REAL)
                    {
                        float real_min = 0.0;
                        float real_max = 1.0;

                        if(column_obj.find("real_min") != column_obj.end())
                            real_min = column_obj.find("real_min").value().get<float>();
                        if(column_obj.find("real_max") != column_obj.end())
                            real_max = column_obj.find("real_max").value().get<float>();

                        column_metadata_obj = new normal_real_column(*prng, *table_metadata_obj,
                            column_name, real_min, real_max);
                    }
                    else if(column_type == redgene_types::DATE)
                    {
                        string ref_user_date = redgene::UNIX_TIME_EPOCH;
                        uint_fast8_t range_in_years = 10;

                        if(column_obj.find("start_date") != column_obj.end())
                            ref_user_date = column_obj.find("start_date")
                                .value().get<string>();
                        if(column_obj.find("range_in_years") != column_obj.end())
                            range_in_years = column_obj.find("range_in_years")
                                .value().get<uint_fast8_t>();

                        column_metadata_obj = new normal_date_column(*prng, *table_metadata_obj,
                            column_name, ref_user_date, range_in_years);
                    }
                    else if(column_type == redgene_types::TIMESTAMP)
                    {
                        string ref_user_date = redgene::UNIX_TIME_EPOCH;
                        uint_fast8_t range_in_years = 10;

                        if(column_obj.find("start_date") != column_obj.end())
                            ref_user_date = column_obj.find("start_date")
                                .value().get<string>();
                        if(column_obj.find("range_in_years") != column_obj.end())
                            range_in_years = column_obj.find("range_in_years")
                                .value().get<uint_fast8_t>();

                        column_metadata_obj = new normal_timestamp_column(*prng, *table_metadata_obj,
                            column_name, ref_user_date, range_in_years);
                    }

                    table_metadata_obj->insert_column_metadata_obj(column_name, column_metadata_obj);
                }
                schema_map.insert(pair<string, table*>(table_name, table_metadata_obj));
                if(comp_pk_attrib_map)
                {
                    for(auto col_attrib_item : *comp_pk_attrib_map)
                    {
                        delete col_attrib_item.second;
                    }
                    delete comp_pk_attrib_map;
                    comp_pk_attrib_map = nullptr;
                    is_comp_pk_map_available = false;
                }
            }
        }

        void datagen()
        {
            //iterate over tables
            for(auto table_obj : schema_map)
            {
                ofstream flatfile(table_obj.first+".csv", std::ofstream::trunc);

                auto table_metadata_obj = table_obj.second;
                auto column_map = table_metadata_obj->get_column_map();
                auto column_order = table_metadata_obj->get_column_order();

                for(uint_fast64_t i = 0; i < table_metadata_obj->get_row_count(); ++i)
                {
                    for(auto itr = column_order.begin(); itr != column_order.end(); ++itr)
                    {
                        if(column_map[*itr]->get_type() == redgene_types::INT)
                        {
                            if(column_map[*itr]->get_constraint_type() == constraints::COMP_PK)
                                flatfile << dynamic_cast<comp_pk_int_column*>(column_map[*itr])->yield();
                            else if(column_map[*itr]->get_constraint_type() == constraints::COMP_FK)
                                flatfile << dynamic_cast<comp_fk_int_column*>(column_map[*itr])->yield();
                            else
                                flatfile << dynamic_cast<normal_int_column*>(column_map[*itr])->yield();
                        }
                        else if(column_map[*itr]->get_type() == redgene_types::STRING)
                        {
                            if(column_map[*itr]->get_constraint_type() == constraints::COMP_PK)
                                flatfile << dynamic_cast<comp_pk_string_column*>(column_map[*itr])->yield();
                            else if(column_map[*itr]->get_constraint_type() == constraints::COMP_FK)
                                flatfile << dynamic_cast<comp_fk_string_column*>(column_map[*itr])->yield();
                            else
                                flatfile << dynamic_cast<normal_string_column*>(column_map[*itr])->yield();
                        }
                        else if(column_map[*itr]->get_type() == redgene_types::REAL)
                            flatfile << dynamic_cast<normal_real_column*>(column_map[*itr])->yield();
                        else if(column_map[*itr]->get_type() ==  redgene_types::DATE)
                            flatfile << dynamic_cast<normal_date_column*>(column_map[*itr])->yield();
                        else if(column_map[*itr]->get_type() ==  redgene_types::TIMESTAMP)
                            flatfile << dynamic_cast<normal_timestamp_column*>(column_map[*itr])->yield();
                        
                        if(column_order.end() - itr != 1)
                            flatfile << '|';
                    }
                    flatfile << '\n';
                }

                if(flatfile.is_open())
                {
                    flatfile.flush();
                    flatfile.close();
                }
            }
        }

        skewness get_skewness_type(const string& skew_string)
        {
            skewness skew;
            if(skew_string == "NO")
                skew = skewness::NO;
            else if(skew_string == "LOW")
                skew = skewness::LOW;
            else if(skew_string == "MEDIUM")
                skew = skewness::MEDIUM;
            else if(skew_string == "HIGH")
                skew = skewness::HIGH;
            else if(skew_string == "EXTREME")
                skew = skewness::EXTREME;
            
            return skew;
        }

        redgene_types get_redgene_type(const string& type_string)
        {
            redgene_types type;
            if(type_string == "INT")
                type = redgene_types::INT;
            else if(type_string == "REAL")
                type = redgene_types::REAL;
            else if(type_string == "STRING")
                type = redgene_types::STRING;
            else if(type_string == "DATE")
                type = redgene_types::DATE;
            else if(type_string == "TIMESTAMP")
                type = redgene_types::TIMESTAMP;
            
            return type;
        }

        constraints get_redgene_constraint(const string& constraint_string)
        {
            constraints constraint;
            if(constraint_string == "PK")
                constraint = constraints::PK;
            else if(constraint_string == "FK")
                constraint = constraints::FK;
            else if(constraint_string == "FK_UNIQUE")
                constraint = constraints::FK_UNIQUE;
            else if(constraint_string == "COMP_PK")
                constraint = constraints::COMP_PK;
            else if(constraint_string == "COMP_FK")
                constraint = constraints::COMP_FK;
            
            return constraint;
        }
    };

}