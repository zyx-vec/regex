#include <iostream>
#include <stack>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <map>
using namespace std;

map<char, int> priority;
#define Split 127
#define Match 0
map<char, int> Map;
const int Left = 0x2, Right = 0x1, Double = 0x3, None = 0x0;

class State {
public:
	State() {}
	State(char c, State *out, State *out1): _c(c), _out(out), _out1(out1) {}
	void setOut(State *out, State *out1);
	void setOut(State *out);
	void setOut1(State *out1);
	char getType() { return _c; }
	State* getOut() { return _out; }
	State* getOut1() { return _out1; }
private:
	State *_in, *_out, *_out1;
	/*0-126, 127, 128*/
	char _c;
};
void State::setOut(State *out, State *out1) {
	this->_out = out;
	this->_out1 = out1;
}
void State::setOut(State *out) {
	this->_out = out;
}
void State::setOut1(State *out1) {
	this->_out1 = out1;
}

class Frag {
public:
	~Frag() { start = NULL; outs.clear(); }
	void setStart(State *s) {
		start = s;
	}
	void setOutNode(State *o) {
		outs.push_back(o);
	}
	// cling e1 and e2's outs togther, copy to e, need to set a new State as start.
	void append(Frag* e1, Frag* e2) {
		for(vector<State*>::iterator i = (e1->outs).begin();\
			i != (e1->outs).end(); i++) {
			outs.push_back(*i);
		}
		for(vector<State*>::iterator i = (e2->outs).begin();\
			i != (e2->outs).end(); i++) {
			outs.push_back(*i);
		}
	}
	// attach e1 e2, and copy start and outs to e.
	void patch(Frag *e1, Frag *e2) {
		State *e2s = e2->getStart();
		for(vector<State*>::iterator i = (e1->outs).begin();\
			i != (e1->outs).end(); i++) {
			if ((*i)->getType() == Split) {
				(*i)->setOut1(e2s);
			} else {
				(*i)->setOut(e2s);
			}
		}
		start = e1->getStart();
		for(vector<State*>::iterator i = (e2->outs).begin();\
			i != (e2->outs).end(); i++) {
			outs.push_back(*i);
		}
	}
	State* getStart() {
		return start;
	}
	State* getEnd() { return outs[0]; }
private:
	State *start;
	vector<State*> outs;
};

void init() {
	priority['*'] = 3; priority['.'] = 2;
	priority['|'] = 1; priority['('] = 0;
	priority['?'] = -1;

	Map['*'] = Left;
	Map['|'] = Double;
	Map['('] = Right;
	Map[')'] = Left;
	for (char i = 'a'; i <= 'z'; i++) {
		Map[i] = None;
	}
}

char* concat(char* re) {
	char *res = (char*) malloc(1024 * sizeof(char));
	char *curr = re;	// char re[] = "a*.(b|c|x)*.f.d";
	
	int count = 0;
	for (; *(curr+1); curr++) {
		if ((Map[*curr] & Right) | (Map[*(curr+1)] & Left)) {
			res[count++] = *curr;
		} else {
			res[count++] = *curr;
			res[count++] = '.';
		}
	}
	res[count] = *curr;
	cout << "concat: " << res << endl;
	return res;
}

// | * concatenation support.
// convert regular expression to reverse Polish regular expression.
vector<char> re2post(char* re) {
	vector<char> rpn;

	char curr;
	stack<char> ops;
	ops.push('?');
	char* s = re;
	for(; *s; s++) {
		switch(*s) {
			case '*':
			case '.':
			case '|':
				curr = ops.top();
				while(priority[curr] >= priority[*s]) {
					rpn.push_back(curr);
					ops.pop();
					curr = ops.top();
				}
				ops.push(*s);
				break;
			case '(':
				ops.push(*s);
				break;
			case ')':
				curr = ops.top();
				while(curr != '(') {
					rpn.push_back(curr);
					ops.pop();
					curr = ops.top();
				}
				ops.pop();
				break;
			default :
				rpn.push_back(*s);
				break;
		}
	}

	while(!ops.empty()) {
		rpn.push_back(ops.top());
		ops.pop();
	}

	return rpn;
}

State* convert2NFA(vector<char>& re) {
	stack<Frag*> frag;
	Frag *e1, *e2, *e;
	State *s;
	vector<char>::iterator iter = re.begin();
	while(iter != re.end()) {
		switch(*iter) {
			case '*':	// e*
				e1 = frag.top(); frag.pop();
				e2 = new Frag();
				e = new Frag();
				s = new State(Split, e1->getStart(), NULL);
				e2->setStart(s);
				e2->setOutNode(s);
				e->patch(e1, e2);	// e copy e1 and set its out node to s.
				e->setStart(s);		// s as start.
				frag.push(e);
				delete e1, e2;
				break;
			case '.':	// e1e2		// start, connect
				e2 = frag.top(); frag.pop();
				e1 = frag.top(); frag.pop();
				e = new Frag();
				e->patch(e1, e2);					// e copy e1 and set its out nodes to e2's start.
				e->setStart(e1->getStart());		// set e1's start as start. e2's out as out node list.
				frag.push(e);
				delete e1, e2;
				break;
			case '|':	// e1|e2
				e1 = frag.top(); frag.pop();
				e2 = frag.top(); frag.pop();
				e = new Frag();
				e->append(e1, e2);
				s = new State(Split, e1->getStart(), e2->getStart());
				e->setStart(s);
				frag.push(e);
				delete e1, e2;
				break;
			default :	// minimum fragment
				s = new State(*iter, NULL, NULL);
				e = new Frag();
				e->setOutNode(s);	// track out nodes
				e->setStart(s);
				frag.push(e);
				break;
		}
		iter++;
	}	
	e = new Frag();
	e1 = frag.top();
	s = new State(Match, NULL, NULL);
	e2 = new Frag();
	e2->setStart(s);
	e = new Frag();
	e->patch(e1, e2);

	delete e1, e2;
	return e->getStart();
}

bool isMatch(string& str, State* start) {
	int len = str.size();
	vector<State*> v, v1;
	v.push_back(start);
	for(int num = 0; num < len; ) {
		for(vector<State*>::iterator i = v.begin();\
			i != v.end(); i++) {
			if(str[num] == (*i)->getType()) {
				cout << "match: " << (*i)->getType() << str[num] << endl;
				v1.push_back((*i)->getOut());
				num++;
			} else if (Split == (*i)->getType()) {
				v1.push_back((*i)->getOut());
				v1.push_back((*i)->getOut1());
			}
		}
		v.clear();
		for(vector<State*>::iterator i = v1.begin();\
			i != v1.end(); i++) {
			v.push_back(*i);
		}
		v1.clear();
		if (v.size() == 0) {	// can't move any more.
			return false;
		}
	}

	for(vector<State*>::iterator i = v.begin();\
		i != v.end(); i++) {
		if ((*i)->getType() == Match) {
			return true;
		}
	}

	return false;
}

int main() {
	init();

	char re[] = "a*.(b|c|x)*.f.d";
	char ret[] = "(a*)*|(b|c|x)*f*d";
	char *polish = concat(ret);	// add concanation operator.
	vector<char> rpn = re2post(re);
	rpn.pop_back();
	cout << "RPN: ";
	for(int i = 0; i < rpn.size(); i++) {
		cout << rpn[i];
	}
	cout << endl;
	State *start;
	start = convert2NFA(rpn);

	
	string str = "aabcxbfd";
	if (isMatch(str, start)) {
		cout << "match" << endl;
	} else {
		cout << "doesn't match" << endl;
	}
	
	return 0;
}
