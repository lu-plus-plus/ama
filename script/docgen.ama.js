'use strict';
const path=require('path');
const fs=require('fs');
const fsext=require('fsext');
const depends=require('depends');
const classes=require('class');

let g_templates={};
function ApplyMDTemplate(s_template,params){
	return s_template.replace(/【(.*?)】/g,(match,pname)=>(params[pname]||'').toString());
}

function CleanupDocString(s){
	let paragraphs=[];
	let cur_paragraph=[];
	let in_code=0;
	let in_list=0;
	for(let line of s.split('\n')){
		if(line.match(/^[* \t/]+$/)){continue;}
		if(line==='/*'||line==='*/'){continue;}
		line=line.replace(/^[ \t]*[/]+/,'');
		if(line.indexOf('```')>=0){
			if(cur_paragraph.length){
				paragraphs.push(cur_paragraph.join(' ').replace(/[ \t]+/,' ').trim()+'\n');
				cur_paragraph.length=0;
			}
			in_code=!in_code;
			if(!in_code){
				paragraphs.push(line+'\n');
				continue;
			}
		}
		if(line.startsWith('- ')){
			if(cur_paragraph.length){
				paragraphs.push(cur_paragraph.join(' ').replace(/[ \t]+/,' ').trim()+'\n');
				cur_paragraph.length=0;
			}
			paragraphs.push(line);
			in_list=1;
			continue;
		}
		if(in_code){
			paragraphs.push(line);
			continue;
		}
		if(in_list){
			paragraphs.push('');
			in_list=0;
		}
		if(!line){
			if(cur_paragraph.length){
				paragraphs.push(cur_paragraph.join(' ').replace(/[ \t]+/,' ').trim()+'\n');
				cur_paragraph.length=0;
			}
			continue;
		}
		cur_paragraph.push(line);
	}
	return paragraphs.join('\n');
}

function CreateNodeAPIItem(name,nd_func,doc){
	let nd_paramlist=nd_func.Find(N_PARAMETER_LIST);
	if(!nd_paramlist){return;}
	let prototype=[];
	let n_optionals=0;
	let n_tmp_optionals=0;
	for(let ndi=nd_paramlist.c;ndi;ndi=ndi.s){
		let name=ndi.GetName();
		if(ndi.Find(N_SYMBOL,'...')){
			name='...'+name;
		}
		if(ndi.node_class==N_ASSIGNMENT&&ndi.c.s.node_class!=N_AIR){
			prototype.push('[');
			n_optionals+=1;
		}else if(ndi.comments_before.indexOf('optional')>=0){
			prototype.push('[');
			n_tmp_optionals+=1;
		}
		if(ndi.Prev()){
			if(prototype[prototype.length-1]==']'){
				prototype.pop();
				prototype.push(', ]');
			}else{
				prototype.push(', ');
			}
		}
		prototype.push(name);
		while(n_tmp_optionals>0){
			prototype.push(']');
			n_tmp_optionals-=1;
		}
	}
	n_optionals+=n_tmp_optionals;
	for(let i=0;i<n_optionals;i++){
		prototype.push(']');
	}
	let self='nd';
	if(name=='MatchAny'||name=='MatchDot'){
		nd='Node';
	}
	return {
		prototype:['- `',self,'.',name,'(',prototype.join(''),')`'].join(''),
		description:doc
	}
}

function GenerateDocuments(){
	//for copy-pasting: 【】
	for(let fn_template of fsext.FindAllFiles(path.join(__dirname,'doc_templates'))){
		if(path.extname(fn_template)==='.md'){
			g_templates[path.parse(fn_template).name.toLowerCase()]=fs.readFileSync(fn_template).toString();
		}
	}
	////////////////
	let all_node_fields=[];
	let all_node_apis=[];
	let nd_node_hpp=depends.LoadFile(path.resolve(__dirname, '../src/ast/node.hpp'));
	let nd_node_class=nd_node_hpp.Find(N_CLASS,'Node');
	let desc=nd_node_class.ParseClass();
	for(let item of desc.properties){
		if(item.enumerable&&item.kind=='variable'){
			all_node_fields.push('**',item.name,'** ',CleanupDocString(item.node.ParentStatement().comments_before),'\n');
		}
	}
	let dedup=new Set();
	for(let nd_method of nd_node_class.FindAll(N_FUNCTION)){
		let name_i=nd_method.data;
		let doc_i=CleanupDocString(nd_method.ParentStatement().comments_before);
		if(!name_i){continue;}
		if(name_i=='LastChildSP'){continue;}
		all_node_apis.push(CreateNodeAPIItem(name_i,nd_method,doc_i));
		dedup.add(name_i);
	}
	all_node_apis.push({separator:'js',description:'---'});
	let nd_init_js=depends.LoadFile(path.resolve(__dirname, '../modules/_init.js'));
	for(let match of nd_init_js.MatchAll(nAssignment(Node.MatchDot(@(Node),'name'),Node.MatchAny('impl')))){
		let name_i=match.name.data;
		if(dedup.has(name_i)){continue;}
		if(name_i=='dfsMatch'){continue;}
		let doc_i=CleanupDocString(match.nd.ParentStatement().comments_before);
		//console.log(match.impl.dump());
		all_node_apis.push(CreateNodeAPIItem(name_i,match.impl,doc_i));
	}
	//dedup - allow functions to share documentation
	let all_node_apis_cpp=undefined;
	let all_node_apis_dedup=[];
	for(let item of all_node_apis){
		if(!item){continue;}
		if(!item.description&&all_node_apis_dedup.length){
			all_node_apis_dedup[all_node_apis_dedup.length-1].prototype=all_node_apis_dedup[all_node_apis_dedup.length-1].prototype+'\n'+item.prototype;
		}else{
			if(item.description==='---'){
				all_node_apis_cpp=all_node_apis_dedup;
				all_node_apis_dedup=[];
				continue;
			}
			all_node_apis_dedup.push(item);
		}
	}
	fs.writeFileSync(
		path.resolve(__dirname, '../doc/api_node.md'),
		ApplyMDTemplate(g_templates.node,{
			fields:all_node_fields.join(''),
			apis_cpp:all_node_apis_cpp.map(item=>ApplyMDTemplate(g_templates.node_api,item)).join(''),
			apis_js:all_node_apis_dedup.map(item=>ApplyMDTemplate(g_templates.node_api,item)).join(''),
			//ctor:
		})
	);
	/////////////////////
	//TODO: module reference
}

module.exports=GenerateDocuments;
