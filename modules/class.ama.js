'use strict';
const assert = require('assert');
const depends = require('depends');
//this is a language support module, we mainly extend Node
let classes = module.exports;

let g_not_class = new Set(['static', 'final', 'const', 'public', 'private', 'protected', 'extends', 'implements']);
let g_protections = ['public', 'private', 'protected'];

function ListOwnProperties(properties, nd_class) {
	let nd_scope = nd_class.LastChild();
	let last_appearance = {};
	let lingering_protection = nd_class.data == 'class' ? 'private' : 'public';
	for (let ndi = nd_scope; ndi;) {
		if (ndi.node_class == N_REF && g_not_class.has(ndi.data)) {
			if (ndi.p && ndi.p.node_class == N_LABELED && ndi.p.c == ndi) {
				//it's lingering
				lingering_protection = ndi.data;
			} else {
				last_appearance[ndi.data] = ndi.ParentStatement();
			}
		}
		let protection = lingering_protection;
		for (let prot of g_protections) {
			if (last_appearance[prot] && last_appearance[prot].isAncestorOf(ndi)) {
				protection = prot;
				break;
			}
		}
		if (ndi.node_class == N_REF && (ndi.flags & REF_DECLARED)) {
			let is_static = last_appearance['static'] && last_appearance['static'].isAncestorOf(ndi) || nd_class.data == 'namespace';
			properties.push({
				enumerable: !is_static | 0,
				writable: !(last_appearance['final'] && last_appearance['final'].isAncestorOf(ndi) || last_appearance['const'] && last_appearance['const'].isAncestorOf(ndi)) | 0,
				own: 1,
				shadowed: 0,
				static: is_static | 0,
				protection: protection,
				kind: 'variable',
				node: ndi,
				name: ndi.data
			});
		} else if (ndi.node_class == N_FUNCTION) {
			if (ndi.data) {
				properties.push({
					enumerable: 0,
					writable: 0,
					own: 1,
					shadowed: 0,
					static: 0 | (last_appearance['static'] && last_appearance['static'].isAncestorOf(ndi)),
					protection: protection,
					kind: 'method',
					node: ndi,
					name: ndi.data
				});
			}
			ndi = ndi.PreorderSkipChildren(nd_class);
			continue;
		} else if (ndi.node_class == N_CLASS) {
			properties.push({
				enumerable: 0,
				writable: 0,
				own: 1,
				shadowed: 0,
				static: 1,
				protection: protection,
				kind: 'class',
				node: ndi,
				name: ndi.GetName()
			});
			ndi = ndi.PreorderSkipChildren(nd_class);
			continue;
		}
		ndi = ndi.PreorderNext(nd_scope);
	}
}

///figure out inheritance and property list without caching: nodes could change later
///often we aren't reusing the result anyway
Node.ParseClass = function() {
	if (this.node_class != N_CLASS) {throw new Error('ParseClass is only valid on a class node');}
	//children: before, name, after, scope
	//base classes
	let nd_after = this.c.s.s;
	function PreorderNextSkipping(nd_self, nd_root) {
		if (nd_self.node_class == N_CALL || nd_self.node_class == N_CALL_TEMPLATE ||
		nd_self.node_class == N_RAW || nd_self.node_class == N_SCOPE ||
		nd_self.node_class == N_DOT || nd_self.node_class == N_ITEM) {
			return nd_self.PreorderSkipChildren(nd_root);
		} else {
			return nd_self.PreorderNext(nd_root);
		}
	}
	let base_class_set = new Set();
	for (let ndi = nd_after; ndi; ndi = PreorderNextSkipping(ndi, nd_after)) {
		if ((ndi.node_class == N_REF || ndi.node_class == N_DOT) && !g_not_class.has(ndi.data)) {
			for (let nd_class of ndi.LookupClass()) {
				base_class_set.add(nd_class);
			}
		}
	}
	let base_classes = [];
	base_class_set.forEach(nd_class=>{
		base_classes.push(nd_class)
	});
	//properties: enum base_classes first
	let properties = [];
	for (let nd_class of base_classes) {
		ListOwnProperties(properties, nd_class);
	}
	for (let ppt of properties) {
		ppt.own = 0;
	}
	ListOwnProperties(properties, this);
	//shadowing deduction
	let shadows = new Set();
	for (let i = properties.length - 1; i >= 0; i--) {
		if (shadows.has(properties[i].name)) {
			properties[i].enumerable = 0;
			properties[i].shadow = 1;
		}
		shadows.add(properties[i].name);
	}
	return {
		nd: this,
		base_classes: base_classes,
		properties: properties,
	}
};

function LookupClassInFile(ret, nd_root, nd_name) {
	//COULDDO: match-ness scoring + high score picking, but conservative listing could work better
	let name = nd_name.GetName();
	for (let nd_class of nd_root.FindAll(N_CLASS, name)) {
		//skip forwards
		if (nd_class.LastChild().node_class == N_SCOPE) {
			ret.push(nd_class);
		}
	}
}

///look up a ref / dot node
Node.LookupClass = function() {
	let ret = [];
	for (let fn of depends.ListAllDependency(this.Root(), true)) {
		let nd_root = depends.LoadFile(fn);
		LookupClassInFile(ret, nd_root, this);
	}
	return ret.map(nd=>nd.ParseClass()).filter(desc=>desc.properties.length > 0);
};

///we return the core class and ignore other modifiers
Node.LookupVariableClass = function() {
	assert(this.node_class == N_REF && (this.flags & REF_DECLARED));
	let nd_def = this;
	let nd_stmt = this.ParentStatement();
	for (let ndi = nd_stmt; ndi; ndi = ndi.PreorderNext(nd_stmt)) {
		if (ndi == nd_def) {break;}
		if (ndi.node_class == N_DOT || ndi.node_class == N_REF) {
			let ret = ndi.LookupClass();
			if (ret.length) {
				return ret;
			}
			//skip the entire dot
			ndi = ndi.PreorderLastInside(ndi);
			continue;
		}
	}
	return [];
};
