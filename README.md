# ReDGene
ReDGene - Relational Data Generator is a tool aimed at taking control over the data generation with being able to generate column vectors in a table with required type, interval, length, cardinality, skewness and constraints like Primary Key, Foreign Key, Foreign Key(unique 1:1 mapping), Composite Primary Key and Composite Foreign Key. 

## Installation
Use any C++ compiler which can support C++11 semantics (with GCC 5.X and above since we use get_time function from \<iomanip>)

```bash
git clone https://github.com/jssprasanna/redgene.git
cd redgene
make
```
## Usage 
redgene accepts data generation configuration JSON file in the format that is described later in here.

```bash
./redgene simple_tab_data.json
```

## Data Generation Configuration File
As stated this tool can be used to generate the relational data with the characteristics of our choice, and to enable this we provide the configuration be provided through JSON file in the format described below.

### _Case-1: Suppose we wish to generate two columns in a table with INT type and STRING type (of length 20)._
```json
{
    "prng": "MT19937",
    "seed": 369
    "tables":
    [
        {
            "table_name": "TAB1",
            "row_count": 100000,
            "columns":
            [
                {
                    "column_name": "column1",
                    "type": "INT"
                },
                {
                    "column_name": "column2",
                    "type": "STRING",
                    "length": 20
                }
            ]
        }
    ]
}
```

### _Case-2: Suppose we wish to generate PK column(only INT and STRING) and generate few columns with required cardinality and skewness_
```json
{
    "prng": "MT19937",
    "seed": 369
    "tables":
    [
        {
            "table_name": "TAB1",
            "row_count": 100000,
            "columns":
            [
                {
                    "column_name": "column1",
                    "type": "INT",
                    "constraint": "PK"
                },
                {
                    "column_name": "column2",
                    "type": "STRING",
                    "length": 20,
                    "cardinality": 0.6,
                    "var_length": true
                },
                {
                    "column_name": "column3",
                    "type": "STRING",
                    "length": 32,
                    "cardinality": 0.2,
                    "skewness": "HIGH"
                },
                {
                    "column_name": "column4",
                    "type": "DATE",
                    "start_date": "2015-Jul-27",
                    "range_in_years": 15
                }
            ]
        }
    ]
}
```

### _Case-3: Create a single table with 10 columns with different types_
```json
{
    "prng":"MT19937",
    "seed":100,
    "tables":
    [{
        "table_name": "tab1",
        "row_count": 1000,
        "columns":
        [
            {
                "column_name": "pkcol",
                "constraint": "PK",
                "type": "INT"
            },
            {
                "column_name": "col1",
                "cardinality": 0.9,
                "type": "INT"
            },
            {
                "column_name": "col2",
                "cardinality": 0.8,
                "type": "INT"
            },
            {
                "column_name": "col3",
                "cardinality": 0.5,
                "type": "INT",
                "skewness": "HIGH"
            },
            {   
                "column_name": "col4",
                "type": "REAL"
            },
            {
                "column_name": "col5",
                "type": "REAL",
                "real_min": 11.3,
                "real_max": 13.5
            },
            {
                "column_name": "col6",
                "cardinality": 0.2,
                "type": "STRING",
                "length": 15,
                "skewness": "EXTREME"
            },
            {
                "column_name": "col7",
                "cardinality": 0.3,
                "type": "STRING",
                "length": 25,
                "skewness": "HIGH",
                "var_length": true
            },
            {
                "column_name": "col8",
                "type": "DATE",
                "start_date": "2015-Jan-20",
                "range_in_years": 20
            },
            {
                "column_name": "col9",
                "type": "TIMESTAMP",
                "start_date": "2000-Sep-22 15:34:55",
                "range_in_years": 10
            }
        ]
    }]
}
```

### _Noteable Points on few column-specific JSON attributes:_
1. **Cardinality** can be provided with any value between (0, row_count].

    *   If value is < 1, then _cardinality_ value will be (_cardinality * row_count_).
    
        For example: if _row_count_ = 1000 and _cardinality_ is 0.3 then cardinality = 1000*0.3 = 300.

    *   If value > 1, then _cardinality_ value will be considered as-is.

2. **Skewness** can be provided with below set of values

    * _NONE {default}, LOW, MEDIUM, HIGH, EXTREME_

### _Case-4: Configuration file showing simple PK-FK relationship_
```json
{
    "prng":"MT19937",
    "seed":100,
    "tables":
    [{
        "table_name": "tab1",
        "row_count": 1000,
        "columns":
        [
            {
                "column_name": "pkcol",
                "constraint": "PK",
                "type": "INT"
            },
            {
                "column_name": "col1",
                "cardinality": 0.9,
                "type": "INT"
            },
            {
                "column_name": "col2",
                "cardinality": 0.8,
                "type": "INT"
            },
            {
                "column_name": "col3",
                "cardinality": 0.5,
                "type": "INT",
                "skewness": "HIGH"
            },
            {   
                "column_name": "col4",
                "type": "REAL"
            },
            {
                "column_name": "col5",
                "type": "REAL",
                "real_min": 11.3,
                "real_max": 13.5
            },
            {
                "column_name": "col6",
                "cardinality": 0.2,
                "type": "STRING",
                "length": 15,
                "skewness": "EXTREME"
            },
            {
                "column_name": "col7",
                "cardinality": 0.3,
                "type": "STRING",
                "length": 25,
                "skewness": "HIGH",
                "var_length": true
            },
            {
                "column_name": "col8",
                "type": "DATE",
                "start_date": "2015-Jan-20",
                "range_in_years": 20
            },
            {
                "column_name": "col9",
                "type": "TIMESTAMP",
                "start_date": "2000-Sep-22 15:34:55",
                "range_in_years": 10
            }
        ]
    },
    {
        "table_name" : "tab2",
        "row_count" : 1000,
        "columns":
        [
            {
                "column_name": "pkcol",
                "constraint": "PK",
                "type": "STRING",
                "length": 15,
                "var_length": true
            },
            {
                "column_name": "fkcol",
                "constraint": "FK",
                "ref_tab": "tab1",
                "ref_col": "pkcol"
            },
            {
                "column_name": "col1",
                "cardinality": 0.3,
                "type": "INT"
            },
            {
                "column_name": "col2",
                "cardinality":0.4,
                "type": "STRING",
                "skewness": "EXTREME",
                "length": 50,
                "var_length": true
            }
        ]
    },
    {
        "table_name" : "tab3",
        "row_count" : 10000,
        "columns":
        [
            {
                "column_name": "fkcol",
                "constraint": "FK",
                "ref_tab": "tab2",
                "ref_col": "pkcol",
                "skewness": "MEDIUM"
            },
            {
                "column_name": "col1",
                "type": "REAL",
                "real_min": 100,
                "real_max": 150
            }
        ]
    }]
}
```
### _Case:5 Configuration file showing multi-table PK to single table FK relationship_
```json
{
    "prng": "MT19937",
    "seed": 369,
    "tables":
    [
        {
            "table_name": "tab1",
            "row_count": 50,
            "columns":
            [
                {
                    "column_name": "pkcol",
                    "constraint": "PK",
                    "type": "INT"
                },
                {
                    "column_name": "col1",
                    "type": "STRING",
                    "length": 10,
                    "cardinality": 0.8
                }
            ]
        },
        {
            "table_name": "tab2",
            "row_count": 50,
            "columns":
            [
                {
                    "column_name": "pkcol",
                    "constraint": "PK",
                    "type": "STRING"
                },
                {
                    "column_name": "col1",
                    "type": "REAL",
                    "real_min": 3.5,
                    "real_max": 28.3
                }
            ]
        },
        {
            "table_name": "tab3",
            "row_count": 50,
            "columns":
            [
                {
                    "column_name": "pkcol",
                    "constraint": "PK",
                    "type": "INT"
                },
                {
                    "column_name": "col1",
                    "type": "STRING",
                    "length": 20,
                    "var_length": true,
                    "cardinality": 0.4
                }
            ]
        },
        {
            "table_name": "tab4",
            "row_count": 50,
            "columns":
            [
                {
                    "column_name": "pkcol",
                    "constraint": "PK",
                    "type": "STRING",
                    "length": 15,
                    "var_length": true
                },
                {
                    "column_name": "col1",
                    "type": "STRING",
                    "length": 8,
                    "cardinality": 0.3,
                    "skewness": "EXTREME"
                }
            ]
        },
        {
            "table_name": "tab5",
            "row_count": 15000,
            "columns":
            [
                {
                    "column_name": "fkcol1",
                    "constraint": "FK",
                    "ref_tab": "tab1",
                    "ref_col": "pkcol",
                    "skewness": "LOW"
                },
                {
                    "column_name": "fkcol2",
                    "constraint": "FK",
                    "ref_tab": "tab2",
                    "ref_col": "pkcol",
                    "skewness": "EXTREME"
                },
                {
                    "column_name": "fkcol3",
                    "constraint": "FK",
                    "ref_tab": "tab3",
                    "ref_col": "pkcol",
                    "skewness": "HIGH"
                },
                {
                    "column_name": "fkcol4",
                    "constraint": "FK",
                    "ref_tab": "tab4",
                    "ref_col": "pkcol",
                    "skewness": "MEDIUM"
                }
            ]
        }
    ]
}
```
### _Case:6 Configuration file to create a Composite Primary Key without other table reference_
```json
{
    "prng":"MT19937",
    "seed":369,
    "tables":
    [
        {
            "table_name": "tab1",
            "row_count": 10000,
            "columns":
            [
                {
                    "column_name": "pkcol1",
                    "constraint": "COMP_PK",
                    "type": "INT"
                },
                {
                    "column_name": "pkcol2",
                    "constraint": "COMP_PK",
                    "type": "STRING",
                    "length": 15,
                    "var_length": true
                },
                {
                    "column_name": "pkcol3",
                    "constraint": "COMP_PK",
                    "type": "INT"
                }
            ]
        }
    ]
 }
```
### _Case:7 Configuration file to create a Composite Primary Key with other entity reference_
```json
{
    "prng":"MT19937",
    "seed":369,
    "tables":
    [
        {
            "table_name": "tab1",
            "row_count": 100,
            "columns":
            [
                {
                    "column_name": "pkcol",
                    "constraint": "PK",
                    "type": "INT"
                },
                {
                    "column_name": "col2",
                    "cardinality": 0.4,
                    "type": "STRING",
                    "skewness": "HIGH"
                }
            ]
        },
        {
            "table_name": "tab2",
            "row_count": 1000,
            "columns":
            [
                {
                    "column_name": "pkcol",
                    "constraint": "PK",
                    "type": "INT"
                },
                {
                    "column_name": "col2",
                    "type": "REAL"
                }
            ]
        },
        {
            "table_name": "tab3",
            "row_count": 10000,
            "columns":
            [
                {
                    "column_name": "pkcol",
                    "constraint": "PK",
                    "type": "STRING",
                    "var_length": true,
                    "length": 20
                },
                {
                    "column_name": "col2",
                    "type": "REAL"
                }
            ]
        },
        {
            "table_name": "tab4",
            "row_count": 100000,
            "columns":
            [
                {
                    "column_name": "pkcol1",
                    "constraint": "COMP_PK",
                    "ref_tab": "tab1",
                    "ref_col": "pkcol"
                },
                {
                    "column_name": "pkcol2",
                    "constraint": "COMP_PK",
                    "ref_tab": "tab2",
                    "ref_col": "pkcol"
                },
                {
                    "column_name": "pkcol3",
                    "constraint": "COMP_PK",
                    "ref_tab": "tab3",
                    "ref_col": "pkcol"
                }
            ]
        }
    ]
 }

```
### _Case:8 Create a Composite Primary Key and corresponding Foreign Key relationship_
```json
{
    "prng":"MT19937",
    "seed":369,
    "tables":
    [
        {
            "table_name": "tab1",
            "row_count": 100,
            "columns":
            [
                {
                    "column_name": "pkcol",
                    "constraint": "PK",
                    "type": "INT"
                },
                {
                    "column_name": "col2",
                    "cardinality": 0.4,
                    "type": "STRING",
                    "skewness": "HIGH"
                }
            ]
        },
        {
            "table_name": "tab2",
            "row_count": 1000,
            "columns":
            [
                {
                    "column_name": "pkcol",
                    "constraint": "PK",
                    "type": "INT"
                },
                {
                    "column_name": "col2",
                    "type": "REAL"
                }
            ]
        },
        {
            "table_name": "tab3",
            "row_count": 10000,
            "columns":
            [
                {
                    "column_name": "pkcol",
                    "constraint": "PK",
                    "type": "STRING",
                    "length": 24,
                    "var_length": true
                },
                {
                    "column_name": "col2",
                    "type": "REAL"
                }
            ]
        },
        {
            "table_name": "tab4",
            "row_count": 100000,
            "columns":
            [
                {
                    "column_name": "pkcol1",
                    "constraint": "COMP_PK",
                    "ref_tab": "tab1",
                    "ref_col": "pkcol"
                },
                {
                    "column_name": "pkcol2",
                    "constraint": "COMP_PK",
                    "ref_tab": "tab2",
                    "ref_col": "pkcol"
                },
                {
                    "column_name": "pkcol3",
                    "constraint": "COMP_PK",
                    "ref_tab": "tab3",
                    "ref_col": "pkcol"
                }
            ]
        },
        {
            "table_name": "tab5",
            "row_count": 450000,
            "columns":
            [
                {
                    "column_name": "fkcol1",
                    "constraint": "COMP_FK",
                    "ref_tab": "tab4",
                    "ref_col": "pkcol1"
                },
                {
                    "column_name": "fkcol2",
                    "constraint": "COMP_FK",
                    "ref_tab": "tab4",
                    "ref_col": "pkcol2"
                },
                {
                    "column_name": "fkcol3",
                    "constraint": "COMP_FK",
                    "ref_tab": "tab4",
                    "ref_col": "pkcol3"
                }
            ]
        }
    ]
 }
```

## Contributing
Pull requests are welcome. For any changes, please open an issue first to discuss what you would like to change.