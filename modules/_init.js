//this module is automatically executed by ama::InitScriptEnv()
//the `require` system isn't ready here so don't use it
//@ama ParseCurrentFile().Save()
(function() {
/////////////
Node.setFlags = function(flags) {
	this.flags = flags;
	return this;
}

Node._isRawNode = Node.isRawNode;
Node.isRawNode = function(s0, s1) {
	if (typeof(s0) === 'string') {s0 = s0.charCodeAt(0);}
	if (typeof(s1) === 'string') {s1 = s1.charCodeAt(0);}
	return this._isRawNode(s0, s1);
}

Node.setData = function(data) {
	this.data = data;
	return this;
}

Node.setIndent = function(indent_level) {
	this.indent_level = indent_level;
	return this;
}

Node.setCommentsBefore = function(comments_before) {
	this.comments_before = comments_before;
	return this;
}

Node.setCommentsAfter = function(comments_after) {
	this.comments_after = comments_after;
	return this;
}

//Create `N_CALL` with `nd` as the function and `args` as arguments.
Node.call = function(...args) {
	return nCall.apply(null, [this].concat(args));
}

//Return an enclosure of the current node in `s_brackets`. `s_brackets` can be '[]' or '()'.
Node.enclose = function(s_brackets) {
	return nRaw(this).setFlags((s_brackets.charCodeAt(0) & 0xff) | (s_brackets.charCodeAt(1) & 0xff) << 8);
}

//Chainable syntax sugar for `f(nd, ...args)`.
Node.then = function(f, ...args) {
	let ret = f.apply(null, [this].concat(args));
	if (ret === undefined) {
		ret = this;
	}
	return ret;
}

//Callback for `JSON.stringify(nd)`. The resulting JSON is for human inspection only and not useful when parsed back.
Node.toJSON = function() {
	let children = [];
	for (let ndi = this.c; ndi; ndi = ndi.s) {
		children.push(ndi);
	}
	return {
		"[node_class]": __node_class_names[this.node_class],
		data: this.data || undefined,
		flags: this.flags || undefined,
		indent_level: this.indent_level || undefined,
		comments_before: this.comments_before || undefined,
		comments_after: this.comments_after || undefined,
		"children": children,
	}
}

Node.dfsMatch = function(ret, nd_pattern) {
	if (nd_pattern.node_class === N_NODEOF) {
		let nd_save = nd_pattern.c;
		if (nd_save.node_class === N_REF) {
			//name
			ret[nd_save.data] = this;
		} else if(nd_save.node_class === N_CALL && nd_save.c.s && !nd_save.c.s.s) {
			//N_FOO(name)
			if (this.node_class !== __global[nd_save.GetName()]) {
				return false;
			}
			ret[nd_save.c.s.GetName()] = this;
		} else if(nd_save.node_class === N_DOT) {
			if (this.node_class !== N_DOT) {return false;}
			if (!this.c.dfsMatch(ret, nd_save.c)) {
				return false;
			}
			ret[nd_save.GetName()] = nRef(this.data);
		} else {
			throw new Error('invalid pattern: ' + nd_pattern.dump())
			}
		return true;
	}
	if (this.node_class !== nd_pattern.node_class || this.data !== nd_pattern.data) {
		return false;
	}
	let ndj = nd_pattern.c;
	for (let ndi = this.c; ndi; ndi = ndi.s) {
		if (!ndj) {
			return false;
		}
		if (!ndi.dfsMatch(ret, ndj)) {
			return false;
		}
		ndj = ndj.s;
	}
	if (ndj) {
		return false;
	}
	return true;
}

//Match a code template specified by `nd_pattern`.
//`nd.Match` only checks `nd` itself. `nd.MatchAll` matches the pattern against the entire subtree under `nd` and returns an array of matches.
//
//The returned match objects have the shape `{nd:<matched node>}`
Node.Match = function(nd_pattern) {
	let ret = {nd: this};
	if (this.dfsMatch(ret, nd_pattern)) {
		return ret;
	} else {
		return undefined;
	}
}

Node.MatchAll = function(nd_pattern) {
	return this.FindAll(nd_pattern.node_class, nd_pattern.data).map(nd=>nd.Match(nd_pattern)).filter(ret=>ret);
}

//Wildcards for template matching. You can invoke the patterns with nested `.()` inside `.()`.
//
//MatchAny matches any node of an optional node class and saves the result in the `name` property of the returned match.
//
//For example, this code:
//```Javascript
//let nd_source = .(test(3));
//nd_source.Match(
//    .(test(.(Node.MatchAny("val"))))
//)
//```
//will return `{nd:nd_source,val:nd_source.Find(N_NUMBER, '3')}`.
Node.MatchAny = function(/*optional*/node_class, name) {
	if (name === undefined) {
		//node_class is actually the name
		return nNodeof(nRef(node_class));
	} else {
		return nNodeof(nCall(nRef(__node_class_names[node_class]), nRef(name)));
	}
}

Node.MatchDot = function(nd_object, name) {
	return nNodeof(nd_object.dot(name));
}

//Substitution a match into the code template specified by `nd`.
//`.(foo)` under `nd` will be replaced with `match.foo`.
//
//For example, this code:
//```Javascript
//let nd_source = .(test(.(val)));
//nd_source.Subst({val:nNumber(3)})
//```
//will return `.(test(3))`
Node.Subst = function(match) {
	let nd_ret = this.Clone();
	for (let ndi = nd_ret; ndi; ndi = ndi.PreorderNext(null)) {
		if (ndi.node_class === N_NODEOF) {
			let nd_name = ndi.c;
			if (nd_name.node_class === N_DOT) {
				let nd_subst = nd_name.c.Subst(match).dot(match[nd_name.GetName()].GetName() || '');
				nd_subst.BreakSibling();
				ndi = ndi.ReplaceWith(nd_subst);
			} else {
				if (nd_name.node_class === N_CALL && nd_name.c.s) {nd_name = nd_name.c.s;}
				let nd_subst = match[nd_name.GetName()];
				if (nd_subst) {
					//nd_subst.BreakSibling();
					ndi = ndi.ReplaceWith(nd_subst);
				}
			}
		}
	}
	return nd_ret;
}

//Save the node. `options` can be:
//- A string starting with '.' interpreted as a new extension
//- A string specifying a full path
//- An object with saving options, see [Parsing Options](#-parsing-options). `options.name` is the full path
//For example, `nd.Save('.audit.cpp')` saves the current AST into a new file with the same file name as `nd.Root().data` and extension '.audit.cpp'.
Node.Save = function(/*optional*/options) {
	if (typeof(options) === 'string') {
		if (options.startsWith('.')) {
			options = {change_ext: options};
		} else {
			options = {name: options};
		}
	}
	if (!options) {
		options = __global.default_options;
	}
	if (options.change_ext) {
		//we need to handle .ama.js => .js shenanigans, so remove everything after the *first* dot
		let pdot = this.data.indexOf('.',Math.max(this.data.lastIndexOf('/'),this.data.lastIndexOf('\\'),0));
		if (pdot < 0) {pdot = this.data.length;}
		options.name = this.data.substr(0, pdot) + (options.change_ext.startsWith('.') ? '' : '.') + options.change_ext;
	}
	let content = this.toSource(options);
	let name = options.name || this.data;
	__writeFileSync(name, content);
	return this;
}

//Remove redundant spaces from the AST, if `aggressive` is true, also remove newlines.
Node.StripRedundantPrefixSpace = function(aggressive) {
	function isSpace(s){
		return s&&!s.trim()&&(aggressive||s.indexOf('\n')<0);
	}
	for (let ndi = this; ndi; ndi = ndi.PreorderNext(this)) {
		if(ndi.node_class===N_KEYWORD_STATEMENT&&ndi.data==='#define'){
			ndi=ndi.PreorderSkip();
			continue;
		}
		if (isSpace(ndi.comments_before)){
			if(ndi.p && (
				ndi.p.node_class === N_CALL || ndi.p.node_class === N_CALL_TEMPLATE || 
				ndi.p.node_class === N_BINOP || ndi.p.node_class === N_ASSIGNMENT ||
				ndi.p.node_class === N_PARAMETER_LIST ||
				ndi.node_class===N_PAREN||ndi.node_class===N_SCOPE||ndi.node_class===N_STRING||
				ndi.node_class===N_EXTENSION_CLAUSE
			)) {
				ndi.comments_before = '';
			}else if(aggressive){
				ndi.comments_before = ndi.comments_before.indexOf('\n')>=0?'\n':' ';
			}
		}
		if (isSpace(ndi.comments_after)){
			if(ndi.p && (
				ndi.p.node_class === N_CALL || ndi.p.node_class === N_CALL_TEMPLATE || 
				ndi.p.node_class === N_BINOP || ndi.p.node_class === N_ASSIGNMENT ||
				ndi.p.node_class === N_PARAMETER_LIST ||
				ndi.node_class===N_PAREN||ndi.node_class===N_SCOPE||ndi.node_class===N_STRING
			)) {
				ndi.comments_after = '';
			}else if(aggressive){
				ndi.comments_after = ndi.comments_after.indexOf('\n')>=0?'\n':' ';
			}
		}
	}
	return this;
}

//Perform template substitution. `match_jobs` is an array with objects in the form `{from:Node, to:Node}`.
//Each match of the `from` pattern will be replaced with its parameters substituted into the corresponding `to` pattern with `Node.Subst`.
//
//If `is_forward` is false, do it backwards.
Node.TranslateTemplates = function(match_jobs, is_forward) {
	let nd_root = this;
	for (let ndi = nd_root; ndi; ndi = ndi.PreorderNext(nd_root)) {
		for (let job of match_jobs) {
			let match = ndi.Match(is_forward ? job.from : job.to);
			if (match) {
				for (let param in match) {
					if (param !== 'nd' && match[param].s) {match[param].BreakSibling();}
				}
				ndi = ndi.ReplaceWith((is_forward ? job.to : job.from).Subst(match));
				if (match.nd === nd_root) {
					nd_root = ndi;
				}
				break;
			}
		}
	}
	return nd_root;
}

//Return the name of `N_FUNCTION`. Returns an empty string if the function is unnamed.
Node.GetFunctionNameNode=function() {
	if(this.node_class !== N_FUNCTION){return undefined;}
	let nd_name = this.c;
	if (nd_name.node_class === N_RAW) {
		nd_name = nd_name.LastChild();
	}
	if (nd_name && nd_name.node_class !== N_REF && nd_name.node_class !== N_DOT) {
		return undefined;
	}
	return nd_name;
}

//Create an extension-aware parsing option for `ParseCode`. 
Node.GetCompleteParseOption=function(options){
	return __PrepareOptions(this.Root().data,options);
}

//The default_options tries to be generic enough for any language.
__global.default_options = {
	enable_hash_comment: 0,
	symbols: '!== != && ++ -- -> ... .. :: << <= === == => >= >>> >> || <=> ** .* ->*',
	//we treat # as an identifier character to make C stuff like `#define` more idiosyncratic
	identifier_charset: '0-9A-Za-z_$#',
	number_charset: '0-9bouUlLfFn.eE',
	hex_number_charset: '0-9A-Fa-fx.pPuUlLn',
	exponent_charset: '0-9f',
	//could use 'dgimsuy', but a later JS standard could extend it
	regexp_flags_charset: 'A-Za-z',
	//we don't really look up the Unicode tables: enable_unicode_identifiers means "all high bytes are treated as identifiers"
	enable_unicode_identifiers: 1,
	finish_incomplete_code: 0,
	///////////
	parse_operators: 1,
	parse_pointed_brackets: 1,
	parse_scoped_statements: 1,
	parse_keyword_statements: 1,
	parse_colon_statements: 1,
	parse_cpp11_lambda: 1,
	parse_declarations: 1,
	parse_cpp_declaration_initialization: 1,
	parse_c_conditional: 1,
	parse_labels: 1,
	parse_air_object: 1,
	parse_indent_as_scope: 0,
	parse_c_forward_declarations: 1,
	struct_can_be_type_prefix: 1,
	parse_js_regexp: 1,
	///////////
	//the 'of' operator is a hack to improve JS for-of parsing
	//each \n denotes a change of priority level, it must be followed by a ' '
	binary_operators: '||\n &&\n |\n ^\n &\n == != === !==\n < <= > >= in of instanceof\n <=>\n << >> >>>\n + -\n * / %\n **\n as\n .* ->*\n',
	prefix_operators: '++ -- ! ~ + - * && & typeof void delete sizeof await co_await new const volatile unsigned signed long short',
	postfix_operators: 'const volatile ++ --',
	cv_qualifiers: 'const volatile',
	//the JS `void` is too common in C/C++ to be treated as an operator by default
	named_operators: 'typeof delete sizeof await co_await new in of instanceof as const volatile',
	//unlike general named_operators, c_type_prefix_operators only make sense when used before another identifier
	c_type_prefix_operators: 'unsigned signed long short',
	ambiguous_type_suffix: '* ** ^ & &&',
	///////////
	keywords_class: 'class struct union namespace interface impl trait',
	keywords_scoped_statement: 'enum if for while do try switch',
	keywords_extension_clause: 'until else elif except catch finally while',
	keywords_function: 'extern function fn def inline',
	keywords_after_class_name: ': extends implements for where',
	keywords_after_prototype: ': -> => throw const noexcept override',
	keywords_not_a_function: 'switch case #define #if #else #elif return',
	keywords_not_variable_name: 'static const volatile private public protected final noexcept throw override virtual operator',
	//case is better treated as a part of a label
	//`template` is parsed by the non-scoped statement parser, but it's created as N_SCOPED_STATEMENT
	keywords_statement: 'return typedef using throw goto #pragma #define #undef #if #ifdef #ifndef #elif #else #endif break continue template',
	keywords_operator_escape: 'operator',
	///////////
	//codegen
	tab_width: 4,
	tab_indent: 2, //2 for auto
	auto_space: 1,
	auto_curly_bracket: 0,
};

__global.extension_specific_options = {
	'.py': Object.assign(Object.create(__global.default_options), {
		enable_hash_comment: 1,
		parse_indent_as_scope: 1,
		parse_js_regexp: 0,
		auto_curly_bracket: 0,
		parser_hook:function(nd_root){
			//parse: Python lambda
			for(let nd_lambda of nd_root.FindAll(N_REF,'lambda')){
				if(nd_lambda.p.node_class!==N_RAW){continue;}
				let nd_raw=nd_lambda.p;
				if(nd_raw.p.node_class!==N_LABELED){
					//misparsed multi-parameter lambda
					let ndi=nd_raw;
					while(ndi&&ndi.node_class!==N_LABELED){
						ndi=ndi.s;
					}
					if(!ndi){continue;}
					let ndi_prev=ndi.Prev();
					nd_raw.ReplaceUpto(ndi_prev,null);
					let nd_other_args=nd_raw.BreakSibling();
					if(nd_other_args){
						nd_raw.Insert(POS_BACK,nd_other_args);
					}
					let nd_last_param=ndi.c;
					nd_last_param.ReplaceWith(nd_raw);
					nd_raw.Insert(POS_BACK,nd_last_param);
					//COULDDO: add back comma if removed
				}
				let nd_labeled=nd_raw.p;
				let params=[];
				for(let ndi=nd_raw.c.s;ndi;ndi=ndi.s){
					if(ndi.node_class===N_REF){
						params.push(nAssignment(ndi.Clone(),nAir()));
					}
				}
				let nd_body=nd_labeled.c.s;
				nd_labeled.ReplaceWith(nFunction(
					nd_raw.c.Unlink(),
					nParameterList.apply(null,params).setFlags(PARAMLIST_UNWRAPPED).SanitizeCommentPlacement(),
					nSymbol(':').setCommentsBefore(nd_raw.comments_after),
					nd_body.Unlink()
				))
			}
			return nd_root;
		}
	}),
}

__global.__PrepareOptions = function(filename, options) {
	let pdot = filename.lastIndexOf('.');
	let proto = __global.extension_specific_options[filename.substr(pdot).toLowerCase()];
	if (proto) {
		let ret = Object.create(proto);
		if (options) {
			for (let key in options) {
				ret[key] = options[key];
			}
		}
		return ret;
	}
	return options;
}


//for instanceof
function fake_options_ctor() {}
fake_options_ctor.prototype = __global.default_options;

//__InheritOptions is called from native code to sanitize option objects
__global.__InheritOptions = function(options) {
	if (!options || options === __global.default_options) {return Object.create(__global.default_options);}
	if (options instanceof fake_options_ctor) {
		return options;
	} else {
		return Object.assign(Object.create(__global.default_options), options);
	}
}

__global.process = {
	env: new Proxy({}, {
		get: function(target, key) {
			return __getenv(key);
		}
	}),
	platform: __platform,
	chdir: __chdir,
	cwd: __cwd
};

__global.__RequireNativeLibrary = function(exports, module, __filename, __dirname) {
	let handle = NativeLibrary.load(__filename);
	module.handle = handle;
	let name = JSON.parse(__path_parse(__filename)).name.toLowerCase();
	if (__platform !== 'win32' && name.startsWith('lib')) {name = name.substr(3);}
	let code = handle.run('AmaInit_' + name, module);
	if (code !== 0) {
		throw new Error(['native module ', JSON.stringify(__filename), ' failed with code ', JSON.stringify(code)].join(''));
	}
}

/////////////
})();
