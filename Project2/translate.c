#ifndef TRANSLATE_C
#define TRANSLATE_C

#define trans( x ) void translate_T_##x
#define translate( x , ... ) translate_T_##x( T_##x * _, ##__VA_ARGS__ )
#define transdecl( x , ... ) void translate( x, ##__VA_ARGS__)
#define transdecl_sizet( x , ... ) size_t translate( x, ##__VA_ARGS__)
#define transcall( x , ... ) translate_T_##x( __VA_ARGS__ )
#define N( x ) T_##x

transdecl(Program);
transdecl(ExtDefList);
transdecl(ExtDef);
transdecl(ExtDecList);
transdecl(Specifier);
transdecl(StructSpecifier);
transdecl(OptTag);
transdecl(Tag);
transdecl_sizet(VarDec, size_t);
transdecl_sizet(VarDimList, size_t);
transdecl(FunDec);
transdecl(VarList);
transdecl(ParamDec);
transdecl(CompSt);
transdecl(StmtList, int); // after exec
transdecl(Stmt);
transdecl(DefList);
transdecl(Def);
transdecl(DecList);
transdecl(Dec);
transdecl(Exp, int, int); // truelabel falselabel
transdecl(Args);

///////////////////////

transdecl(Program) {transcall(ExtDefList, _->programBody );}

transdecl(ExtDefList)
{
	foreach(_, extdef, N(ExtDef))
	{
		transcall(ExtDef, extdef);
	}
}

transdecl(ExtDef)
{
	if (_->function) // function
	{
		sprintf(stderr, "function\n");
		int son_cnt = 0;
		foreach ( _->function->varList , vlist , VarList ) son_cnt++;
		// 计算函数参数个数
		size_t func_type = E_trie_find(_->spec->typeName);
		// 函数被定义了
		if (E_trie_find(_->function->name))
		{
			fprintf(stderr, "Redeclare of function %s\n", _->function->name);
			return;
		}
		// TODO: 处理包含结构体定义的情况
		if (!func_type)
		{
			fprintf(stderr, "Unknown type %s\n", _->spec->typeName);
			return;
		}
		size_t func_symbol_item = E_symbol_table_new();
		E_symbol_table[func_symbol_item].type_uid = func_type;
		E_symbol_table[func_symbol_item].is_abstract = EJQ_SYMBOL_FUNCTION;
		E_symbol_table[func_symbol_item].name = _->function->name;
		E_trie_insert(_->function->name, func_symbol_item);
		// 创建函数语法树节点
		size_t current_version = E_trie_get_current_version();
		// 备份当前trie树版本号
		E_symbol_table[func_symbol_item].son_cnt = son_cnt;
		E_symbol_table[func_symbol_item].son = (size_t*)malloc(sizeof(size_t) * son_cnt);
		int ti = 0;
		foreach ( _->function->varList, vlist, VarList )
		{
			size_t func_arg_symbol = E_symbol_table_new();
			E_symbol_table[func_symbol_item].son[ti] = func_arg_symbol;
			E_symbol_table[func_arg_symbol].type_uid = E_trie_find(vlist->thisParam->type->typeName);
			if (!E_symbol_table[func_arg_symbol].type_uid)
			{
				fprintf(stderr, "Unknown type %s\n", vlist->thisParam->type->typeName);
				return;
			}
		}
		// 翻译函数体
		transcall(CompSt, _->functionBody);
		// 恢复trie树版本
		E_trie_back_to_version(current_version);
	} else // variables
	{
		size_t vari_type = E_trie_find(_->spec->typeName);
		if (!vari_type)
		{
			fprintf(stderr, "Unknown type %s\n", _->spec->typeName);
			return;
		}
		// TODO: 处理包含结构体定义的情况
		// 获取变量类型号
		foreach ( _->dec, decList, ExtDecList )
		{
			// 单独处理每个变量
			if (E_trie_find(decList->dec->varName))
			{
				// 可持久化Trie树
				// 这是全局变量，所以直接查询当前的版本有没有定义就好了
				fprintf(stderr, "Redeclare of %s\n", decList->dec->varName);
				continue;
			}
			// 计算这个变量
			size_t id = transcall(VarDec, decList->dec, vari_type);
			// 加入到trie树中
			E_trie_insert(decList->dec->varName, id);
		}
	}
}

transdecl_sizet(VarDec, size_t spec)
{
	size_t typeid = spec;
	if (_->dim)
	{
		typeid = transcall(VarDimList, _->dim, spec);
	}
	size_t thisid = E_symbol_table_new();
	E_symbol_table[thisid].type_uid = typeid;
	E_symbol_table[thisid].name = _->varName;
	E_symbol_table[thisid].len = E_symbol_table[typeid].len;
	E_symbol_table[thisid].is_abstract = 0x002;
	E_symbol_table[thisid].offset = 0;
	E_symbol_table[thisid].son_cnt = 0;
	E_symbol_table[thisid].son = NULL;
	return thisid;
}

transdecl_sizet(VarDimList, size_t spec)
{
	size_t thisdim = spec;
	size_t thisid = E_symbol_table_new();
	char buf[512];
	if (_->next)
	{
		thisdim = transcall(_->next, spec);
		sprintf(buf, "%s/a", E_symbol_table[thisdim].name);
	} else {
		sprintf(buf, "%d/a", (int)spec);
	}
	E_symbol_table[thisid].type_uid = thisdim;
	E_symbol_table[thisid].name = (char *)malloc(strlen(buf) + 5);
	for (size_t i = 0; buf[i]; i++)
		E_symbol_table[thisid].name[i] = buf[i];
	E_symbol_table[thisid].len = E_symbol_table[typeid].len * _->thisDim;
	E_symbol_table[thisid].is_abstract = 0x004;
	E_symbol_table[thisid].offset = 0;
	E_symbol_table[thisid].son_cnt = 0;
	E_symbol_table[thisid].son = NULL;
	return thisid;
}


#undef trans
#undef transdecl
#undef translate
#undef N

#endif