# JSON module to read json file into a dict object.
import json
import sys
from itertools import groupby

def sort_func(obj):
	return obj['ref_tab']

if(len(sys.argv) != 2):
	print("Usage: get_redgene_ddls <dgen_template_json>\n")
	exit(-1)

jfh = open(sys.argv[1], "r")
sfh = open(sys.argv[1].split('.')[0]+".sql", "w")

dgen_json = json.load(jfh)

def get_ref_col_type(json_handle, ref_tab, ref_col):
	for i in range(0, len(json_handle['tables'])):
		if(json_handle['tables'][i]['table_name'] == ref_tab):
			for j in range(0, len(dgen_json['tables'][i]['columns'])):
				if(dgen_json['tables'][i]['columns'][j]['column_name'] == ref_col):
					if(dgen_json['tables'][i]['columns'][j].get('type') is not None):
						if(dgen_json['tables'][i]['columns'][j]['type'] == 'DATE'):
							return 'CHAR(11) DATE_FORMAT DATE MASK "DD-MON-YYYY"'
						elif(dgen_json['tables'][i]['columns'][j]['type'] == 'TIMESTAMP'):
							return 'CHAR(20) DATE_FORMAT TIMESTAMP MASK "DD-MON-YYYY HH24:MI:SS"'
						elif(dgen_json['tables'][i]['columns'][j]['type'] == 'STRING'):
							if(dgen_json['tables'][i]['columns'][j].get('length') is not None):
								if(dgen_json['tables'][i]['columns'][j]['length'] > 255):
									return ' char(' + str(dgen_json['tables'][i]['columns'][j]['length']) +')'			
					else:
						get_ref_col_type(json_handle, dgen_json['tables'][i]['columns'][j]['ref_tab'], dgen_json['tables'][i]['columns'][j]['ref_col'])


for i in range(0, len(dgen_json['tables'])):
	et_column_list = ''
	crt_tab_ddl = 'create table '+dgen_json['tables'][i]['table_name']+'_et('
	for j in range(0, len(dgen_json['tables'][i]['columns'])):
		crt_tab_ddl = crt_tab_ddl + '\n\t' + dgen_json['tables'][i]['columns'][j]['column_name']
		if(dgen_json['tables'][i]['columns'][j].get('type') is not None):
			if(dgen_json['tables'][i]['columns'][j]['type'] == 'DATE'):
				et_column_list += dgen_json['tables'][i]['columns'][j]['column_name']
				et_column_list += ' CHAR(11) DATE_FORMAT DATE MASK "DD-MON-YYYY"'
			elif(dgen_json['tables'][i]['columns'][j]['type'] == 'TIMESTAMP'):
				et_column_list += dgen_json['tables'][i]['columns'][j]['column_name']
				et_column_list += ' CHAR(20) DATE_FORMAT TIMESTAMP MASK "DD-MON-YYYY HH24:MI:SS"'
			elif(dgen_json['tables'][i]['columns'][j]['type'] == 'STRING'):
				if(dgen_json['tables'][i]['columns'][j].get('length') is not None):
					if(dgen_json['tables'][i]['columns'][j]['length'] > 255):
						et_column_list += dgen_json['tables'][i]['columns'][j]['column_name']
						et_column_list += ' char(' + str(dgen_json['tables'][i]['columns'][j]['length']) +')'
					else:
						et_column_list += dgen_json['tables'][i]['columns'][j]['column_name']
				else:
					et_column_list += dgen_json['tables'][i]['columns'][j]['column_name']
			else:
				et_column_list += dgen_json['tables'][i]['columns'][j]['column_name']
			if(j != len(dgen_json['tables'][i]['columns'])-1):
				et_column_list += ',\n\t\t'
		elif(dgen_json['tables'][i]['columns'][j].get('ref_tab') is not None):
			et_column_list += dgen_json['tables'][i]['columns'][j]['column_name']
			if(get_ref_col_type(dgen_json, dgen_json['tables'][i]['columns'][j]['ref_tab'], dgen_json['tables'][i]['columns'][j]['ref_col']) is not None):
				et_column_list += ' ' + get_ref_col_type(dgen_json, dgen_json['tables'][i]['columns'][j]['ref_tab'], dgen_json['tables'][i]['columns'][j]['ref_col'])
	
			if(j != len(dgen_json['tables'][i]['columns'])-1):
				et_column_list += ',\n\t\t'

		if('type' in dgen_json['tables'][i]['columns'][j]):
			if(dgen_json['tables'][i]['columns'][j]['type'] == 'INT' or dgen_json['tables'][i]['columns'][j]['type'] == 'REAL'):
				crt_tab_ddl = crt_tab_ddl+' number'
				if(j != len(dgen_json['tables'][i]['columns'])-1):
					crt_tab_ddl = crt_tab_ddl+','
			elif(dgen_json['tables'][i]['columns'][j]['type'] == 'STRING'):
				crt_tab_ddl = crt_tab_ddl+' varchar2('
				if(dgen_json['tables'][i]['columns'][j].get('length') is not None):
					crt_tab_ddl = crt_tab_ddl+str(dgen_json['tables'][i]['columns'][j]['length'])+')'
				else:
					crt_tab_ddl = crt_tab_ddl+'10)'
				if(j != len(dgen_json['tables'][i]['columns'])-1):
					crt_tab_ddl = crt_tab_ddl+','
			elif(dgen_json['tables'][i]['columns'][j]['type'] == 'DATE'):
				crt_tab_ddl = crt_tab_ddl+' date'
				if(j != len(dgen_json['tables'][i]['columns'])-1):
					crt_tab_ddl = crt_tab_ddl+','
			elif(dgen_json['tables'][i]['columns'][j]['type'] == 'TIMESTAMP'):
				crt_tab_ddl = crt_tab_ddl+' timestamp'
				if(j != len(dgen_json['tables'][i]['columns'])-1):
					crt_tab_ddl = crt_tab_ddl+','
		else:
			crt_tab_ddl = crt_tab_ddl+' number'
			if(j != len(dgen_json['tables'][i]['columns'])-1):
				crt_tab_ddl = crt_tab_ddl+','
	crt_tab_ddl = crt_tab_ddl+')\n'
	crt_tab_ddl = crt_tab_ddl+'organization external(\n'
	crt_tab_ddl = crt_tab_ddl+'\tTYPE ORACLE_LOADER\n'
	crt_tab_ddl = crt_tab_ddl+'\tdefault directory <ora_dir>\n'
	crt_tab_ddl = crt_tab_ddl+'\taccess parameters(\n'
	crt_tab_ddl = crt_tab_ddl+'\t\trecords delimited by newline\n'
	crt_tab_ddl = crt_tab_ddl+'\t\tnobadfile nologfile\n'
	crt_tab_ddl = crt_tab_ddl+"\t\tfields terminated by '|'\n"
	crt_tab_ddl = crt_tab_ddl+'\t\tmissing field values are null\n'
	crt_tab_ddl = crt_tab_ddl+'\t\t('+et_column_list+')'
	crt_tab_ddl = crt_tab_ddl+')\n'
	crt_tab_ddl = crt_tab_ddl+"\tlocation('"+dgen_json['tables'][i]['table_name']+".csv'))"
	crt_tab_ddl = crt_tab_ddl+ '\nreject limit unlimited;\n'

	crt_tab_ddl = crt_tab_ddl+'create table '+dgen_json['tables'][i]['table_name']
	crt_tab_ddl = crt_tab_ddl+' as select * from '+dgen_json['tables'][i]['table_name']+'_et'
	crt_tab_ddl = crt_tab_ddl+';\n\n'
	sfh.write(crt_tab_ddl)

# generate constraint addition DDL statements.
# It would be better to keep this alter table add constraint
# DDLs at the end, so that it will be easy to track constraint types
for i in range(0, len(dgen_json['tables'])):
	comp_pk_nonref_list = []
	comp_pk_list = []
	comp_fk_list = []
	
	for j in range(0, len(dgen_json['tables'][i]['columns'])):
		if('constraint' in dgen_json['tables'][i]['columns'][j]):
			altr_tab_ddl = 'alter table '+dgen_json['tables'][i]['table_name']
			altr_tab_ddl = altr_tab_ddl+' add constraint'
			if(dgen_json['tables'][i]['columns'][j]['constraint'] == 'PK'):
				altr_tab_ddl = altr_tab_ddl+' pk_'+dgen_json['tables'][i]['table_name']
				altr_tab_ddl = altr_tab_ddl+' primary key('
				altr_tab_ddl = altr_tab_ddl+dgen_json['tables'][i]['columns'][j]['column_name']+');\n'
				sfh.write(altr_tab_ddl)
			elif(dgen_json['tables'][i]['columns'][j]['constraint'] == 'FK' or dgen_json['tables'][i]['columns'][j]['constraint'] == 'FK_UNIQUE'):
				altr_tab_ddl = altr_tab_ddl+' fk_'+dgen_json['tables'][i]['table_name']
				altr_tab_ddl = altr_tab_ddl+'_'+dgen_json['tables'][i]['columns'][j]['ref_tab']
				altr_tab_ddl = altr_tab_ddl+' foreign key('
				altr_tab_ddl = altr_tab_ddl+dgen_json['tables'][i]['columns'][j]['column_name']+')'
				altr_tab_ddl = altr_tab_ddl+' references '+dgen_json['tables'][i]['columns'][j]['ref_tab']
				altr_tab_ddl = altr_tab_ddl+'('+dgen_json['tables'][i]['columns'][j]['ref_col']+');\n'
				sfh.write(altr_tab_ddl)
			elif(dgen_json['tables'][i]['columns'][j]['constraint'] == 'COMP_PK'):
				if(dgen_json['tables'][i]['columns'][j].get('ref_tab') is not None):
					comp_pk_list.append(dgen_json['tables'][i]['columns'][j])
				else:
					comp_pk_nonref_list.append(dgen_json['tables'][i]['columns'][j])
			elif(dgen_json['tables'][i]['columns'][j]['constraint'] == 'COMP_FK'):
				comp_fk_list.append(dgen_json['tables'][i]['columns'][j])

	if(len(comp_pk_nonref_list) > 0):
		nonref_pk_names_str = ''
		for k in range(0, len(comp_pk_nonref_list)):
			nonref_pk_names_str = nonref_pk_names_str + comp_pk_nonref_list[k]['column_name']
			if(k != len(comp_pk_nonref_list)-1):
				nonref_pk_names_str = nonref_pk_names_str+','
		altr_tab_ddl = 'alter table '+dgen_json['tables'][i]['table_name']
		altr_tab_ddl = altr_tab_ddl+' add constraint comp_pk_'
		altr_tab_ddl = altr_tab_ddl+dgen_json['tables'][i]['table_name']
		altr_tab_ddl = altr_tab_ddl+' primary key('
		altr_tab_ddl = altr_tab_ddl+nonref_pk_names_str+');\n'
		sfh.write(altr_tab_ddl)

	if(len(comp_pk_list) > 0):
		pk_names_str = ''
		for k in range(0, len(comp_pk_list)):
			pk_names_str = pk_names_str + comp_pk_list[k]['column_name']
			if(k != len(comp_pk_list)-1):
				pk_names_str = pk_names_str+','
			altr_tab_ddl = 'alter table '+dgen_json['tables'][i]['table_name']
			altr_tab_ddl = altr_tab_ddl+' add constraint fk_'
			altr_tab_ddl = altr_tab_ddl+dgen_json['tables'][i]['table_name']+'_'
			altr_tab_ddl = altr_tab_ddl+comp_pk_list[k]['ref_tab']
			altr_tab_ddl = altr_tab_ddl+' foreign key('+comp_pk_list[k]['column_name']+')'
			altr_tab_ddl = altr_tab_ddl+' references '+comp_pk_list[k]['ref_tab']
			altr_tab_ddl = altr_tab_ddl+'('+comp_pk_list[k]['ref_col']+');\n'
			sfh.write(altr_tab_ddl)

		altr_tab_ddl = 'alter table '+dgen_json['tables'][i]['table_name']
		altr_tab_ddl = altr_tab_ddl+' add constraint cpk_'
		altr_tab_ddl = altr_tab_ddl+dgen_json['tables'][i]['table_name']
		altr_tab_ddl = altr_tab_ddl+' primary key('
		altr_tab_ddl = altr_tab_ddl+pk_names_str+');\n'
		sfh.write(altr_tab_ddl)

	if(len(comp_fk_list) > 0):
		comp_fk_list.sort(key=sort_func)
		comp_fk_list = [list(idx) for j, idx in groupby(comp_fk_list, lambda a: a['ref_tab'])]	
		
		for comp_fk_group in range(0, len(comp_fk_list)):
			fk_names_str = ''
			rfk_names_str = ''
			grouped_list = comp_fk_list[comp_fk_group]

			for k in range(0, len(grouped_list)):
				fk_names_str = fk_names_str + grouped_list[k]['column_name']
				rfk_names_str = rfk_names_str + grouped_list[k]['ref_col']
				if(k != len(grouped_list)-1):
					fk_names_str = fk_names_str+','
					rfk_names_str = rfk_names_str+','

			altr_tab_ddl = 'alter table '+dgen_json['tables'][i]['table_name']
			altr_tab_ddl = altr_tab_ddl+' add constraint cfk_'
			altr_tab_ddl = altr_tab_ddl+dgen_json['tables'][i]['table_name']
			altr_tab_ddl = altr_tab_ddl+'_'+grouped_list[0]['ref_tab']
			altr_tab_ddl = altr_tab_ddl+' foreign key('+fk_names_str+')'
			altr_tab_ddl = altr_tab_ddl+' references '+grouped_list[0]['ref_tab']+'('
			altr_tab_ddl = altr_tab_ddl+rfk_names_str+');\n'
			sfh.write(altr_tab_ddl)

print("Written all DDLs to "+sys.argv[1].split('.')[0]+".sql"+ " file.\n")
jfh.close()
sfh.close()
