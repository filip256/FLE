#include <SFML/Graphics.hpp>
#include <Windows.h>
#include <iostream>

#define WINDOW_SIZE_X 1000.0
#define WINDOW_SIZE_Y 600.0

using namespace std;

class Operator
{
public:
	char symbol = 0;
	int precedance = 0, arity;

	Operator()
	{
		symbol = 0;
		precedance = 0;
		arity = 0;
	}
	Operator(const char s, const int p, const int a)
	{
		symbol = s;
		precedance = p;
		arity = a;
	}
};
class Operand
{
public:
	char symbol = 0;
	int value = 0;

	bool isTreeHandle = false; //in case operand is a subtree
	int handle = 0;

	Operand(const char s, const int v)
	{
		symbol = s;
		value = v;
		isTreeHandle = false;
		handle = 0;
	}
	Operand(const int npos)
	{
		symbol = 0;
		value = 0;
		isTreeHandle = true;
		handle = npos;
	}
};

//GUI classes
class TextBox
{
public:
	string textStr;

	sf::Vector2f pos, size;
	sf::Color backgroundCol, textCol;
	sf::RectangleShape background;
	sf::Font font;
	sf::Text text;

	void construct(const sf::Vector2f p, const sf::Vector2f s, const sf::Color bgCol, const sf::Color txCol, const string defText)
	{
		pos = p;
		size = s;
		backgroundCol = bgCol;
		textCol = txCol;

		background.setSize(s);
		background.setPosition(p);
		background.setFillColor(bgCol);

		font.loadFromFile("dosfont.ttf");
		text.setFont(font);
		text.setString(defText);
		text.setFillColor(txCol);
		textCol = txCol;

		text.setCharacterSize(32);
		text.setPosition(p.x + 3, p.y + (s.y - text.getGlobalBounds().height) / 2.0);
	}
	void setText(const string s)
	{
		textStr = s;
		text.setString(textStr);
	}

	void draw(sf::RenderWindow &window)
	{
		window.draw(background);
		window.draw(text);
	}
};
class InputBox
{
public:
	bool isSelected = false;
	string textStr, defTextStr;
	int cursorPos = 0;
	int blink = 0;

	sf::Vector2f pos, size;
	sf::Color backgroundCol, textCol;
	sf::RectangleShape background, cursor;
	sf::Font font;
	sf::Text text;

	void construct(const sf::Vector2f p, const sf::Vector2f s, const sf::Color bgCol, const sf::Color txCol, const string defText)
	{
		pos = p;
		size = s;
		backgroundCol = bgCol;
		textCol = txCol;

		background.setSize(s);
		background.setPosition(p);
		background.setFillColor(bgCol);

		defTextStr = defText;
		font.loadFromFile("dosfont.ttf");
		text.setFont(font);
		text.setString(defText);
		text.setFillColor(sf::Color(bgCol.r + 50, bgCol.g + 50, bgCol.b + 50, bgCol.a));
		text.setStyle(sf::Text::Italic);

		textCol = txCol;

		text.setCharacterSize(32);
		text.setPosition(p.x + 3, p.y + (s.y - text.getGlobalBounds().height) / 2.0);


		cursor.setSize(sf::Vector2f(2.0, 32.0));
		cursor.setFillColor(sf::Color::White);
		cursor.setPosition(p.x + 3, p.y + (s.y - 32) / 2.0);
	}

	bool containsPoint(const sf::Vector2i p)
	{
		if (p.x >= pos.x && p.y >= pos.y && p.x <= pos.x + background.getGlobalBounds().width && p.y <= pos.y + background.getGlobalBounds().height)
			return true;
		return false;
	}
	void moveCursor(const int v)
	{
		if (cursorPos + v >= 0 && cursorPos + v <= textStr.length())
		{
			cursorPos += v;
			cursor.setPosition(pos.x + cursorPos * 18, cursor.getPosition().y);
			blink = 10;
		}
	}
	bool modifyString(const char &c)
	{
		int a = text.getGlobalBounds().width;
		if (c == 8 && textStr.length())
		{
			textStr.erase(cursorPos - 1, 1);
			moveCursor(-1);
		}
		else if (c >= ' ' && c <= '~')
		{
			string aux(1, c);
			textStr.insert(cursorPos, aux);
			moveCursor(1);
		}

		if (textStr.length())
		{
			text.setString(textStr);
			text.setFillColor(textCol);
			text.setStyle(sf::Text::Regular);

			while (text.getGlobalBounds().width > size.x - 20 && text.getCharacterSize() >= 16)
				text.setCharacterSize(text.getCharacterSize() - 2);
			//cursor.setPosition(pos.x + 3 + text.getGlobalBounds().width, pos.y + (size.y - 24) / 2.0);
		}
		else
		{
			text.setString(defTextStr);
			text.setFillColor(sf::Color(backgroundCol.r + 50, backgroundCol.g + 50, backgroundCol.b + 50, backgroundCol.a));
			text.setStyle(sf::Text::Italic);
			cursor.setPosition(pos.x + 3, pos.y + (size.y - 32) / 2.0);
		}

		if (c == 13)
		{
			isSelected = false;
			return 1;
		}
		return 0;
	}

	void draw(sf::RenderWindow &window)
	{
		window.draw(background);
		window.draw(text);

		if (isSelected)
		{
			if (blink < 30)
				window.draw(cursor);

			if (blink < 60)
				++blink;
			else
				blink = 0;
		}
	}
};
class Button
{
public:
	bool isAnimated = false;
	int clicked = 0;

	sf::Vector2f pos;
	sf::Vector2i size;
	sf::Texture tx;
	sf::Sprite sp;

	void construct(const sf::Vector2f p, const sf::Vector2i s, const sf::Vector2f scale, const string location, const bool anim)
	{
		if (!tx.loadFromFile(location))
			cout << "Couldn't open texture file: " << location << '\n';
		else
		{
			isAnimated = anim;
			pos = p;
			size = s;

			sp.setTexture(tx);
			sp.setTextureRect(sf::IntRect(0, 0, s.x, s.y));
			sp.setScale(scale);
			sp.setPosition(p);
		}
	}

	bool containsPoint(const sf::Vector2i p)
	{
		if (p.x >= pos.x && p.y >= pos.y && p.x <= pos.x + sp.getGlobalBounds().width && p.y <= pos.y + sp.getGlobalBounds().height)
			return true;
		return false;
	}

	void onPress()
	{
		sp.setTextureRect(sf::IntRect(size.x, 0, size.x, size.y));
	}
	void onRelease()
	{
		sp.setTextureRect(sf::IntRect(0, 0, size.x, size.y));
	}

	void draw(sf::RenderWindow &window)
	{
		window.draw(sp);
	}
};
class SquareBox
{
public:
	string textStr;

	sf::Vector2f pos, size;
	sf::Color color;
	sf::RectangleShape square;
	sf::Font font;
	sf::Text text;

	SquareBox()
	{
		pos = sf::Vector2f(0.0, 0.0);
		size = sf::Vector2f(50.0, 50.0);
		color = sf::Color(sf::Color::White);
	}
	SquareBox(const sf::Vector2f p, const sf::Vector2f s, const sf::Color col, const string txt)
	{
		pos = p;
		size = s;
		color = col;
		textStr = txt;

		square.setPosition(pos);
		square.setFillColor(color);
		square.setSize(size);
		square.setOutlineThickness(5.0);
		square.setOutlineColor(sf::Color(color.r - 50, color.g - 50, color.b, color.a));

		font.loadFromFile("dosfont.ttf");

		text.setFont(font);
		text.setString(textStr);
		text.setFillColor(sf::Color::Black);

		text.setCharacterSize(32);
		while (text.getGlobalBounds().width > size.x - 20)
			text.setCharacterSize(text.getCharacterSize() - 1);
		text.setPosition(p.x + (s.x - text.getGlobalBounds().width) / 2.0, p.y + 5.0);
	}

	void setPosition(const sf::Vector2f p)
	{
		text.setFont(font); //weird SFML font pointer gets lost and needs to be set again
		pos = p;
		square.setPosition(pos);
		text.setPosition(p.x + (size.x - text.getGlobalBounds().width) / 2.0, p.y + 5.0);
	}

	void draw(sf::RenderWindow &window)
	{
		text.setFont(font); //weird SFML font pointer gets lost and needs to be set again
		window.draw(square);
		window.draw(text);
	}

};
class GraphicStack
{
public:
	vector<SquareBox> boxList;

	void setStack(const vector<Operator> &stack)
	{
		boxList.clear();
		const int size = stack.size();
		for (int i = 0; i < size; ++i)
		{
			boxList.push_back(SquareBox(sf::Vector2f(0.0, 0.0), sf::Vector2f(50.0, 50.0), sf::Color(0, stack[i].precedance * 2 + 50, 255, 255), string(1, stack[i].symbol)));
		}
	}
	void setStack(const vector<Operand> &stack)
	{
		boxList.clear();
		const int size = stack.size();
		for (int i = 0; i < size; ++i)
		{
			if (!stack[i].isTreeHandle)
				boxList.push_back(SquareBox(sf::Vector2f(0.0, 0.0), sf::Vector2f(50.0, 50.0), sf::Color::Yellow, string(1, stack[i].symbol)));
			else
				boxList.push_back(SquareBox(sf::Vector2f(0.0, 0.0), sf::Vector2f(50.0, 50.0), sf::Color::Yellow, "@" + to_string(stack[i].handle)));
		}
	}

	void setPosition(const sf::Vector2f pos)
	{
		const int size = boxList.size();
		for (int i = 0; i < size; ++i)
			boxList[i].setPosition(sf::Vector2f(pos.x, pos.y + i * (boxList[i].size.y + boxList[i].square.getOutlineThickness() + 10)));
	}

	void draw(sf::RenderWindow &window)
	{
		const int size = boxList.size();
		for (int i = 0; i < size; ++i)
			boxList[i].draw(window);
	}

};


//FLAT classes
class StackArchive
{
public:
	int size = 0;
	vector<vector<Operator>> operatorArchive;
	vector<vector<Operand>> operandArchive;

	void push(const vector<Operator> &stack)
	{
		operatorArchive.push_back(stack);
		++size;
	}
	void push(const vector<Operand> &stack)
	{
		operandArchive.push_back(stack);
		++size;
	}
	void draw(const sf::Vector2f pos1, const sf::Vector2f pos2, const  int step, sf::RenderWindow &window)
	{
		GraphicStack gStack;
		gStack.setStack(operatorArchive[step]);
		gStack.setPosition(pos1);
		gStack.draw(window);
		gStack.setStack(operandArchive[step]);
		gStack.setPosition(pos2);
		gStack.draw(window);
	}
	void clear()
	{
		operatorArchive.clear();
		operandArchive.clear();
		size = 0;
	}
};
class Node
{
public:
	char symbol = 0;
	int father = -1, leaf[2] = { -1 };

	sf::CircleShape circle;
	sf::Font font;
	sf::Text text;

	Node(const char s, const int f, const int l1, const int l2)
	{
		symbol = s;
		father = f;
		leaf[0] = l1;
		leaf[1] = l2;

		font.loadFromFile("dosfont.ttf");

		text.setString(std::string(1, s));
		text.setFont(font);
		text.setFillColor(sf::Color::Black);


		circle.setRadius(20.0);
		circle.setPointCount(20);
		circle.setOutlineThickness(2.0);

		if (leaf[0] != -1)
		{
			circle.setFillColor(sf::Color(0, 0, 255, 255));
			circle.setOutlineColor(sf::Color(0, 0, 175, 255));
		}
		else
		{
			circle.setFillColor(sf::Color(255, 255, 0, 255));
			circle.setOutlineColor(sf::Color(175, 175, 0, 255));
		}

		circle.setOrigin(20.0, 20.0);
		circle.setPosition(0, 0);

		text.setOrigin(text.getGlobalBounds().width / 2.0, text.getGlobalBounds().height / 2.0 + 8.0);
		text.setPosition(0, 0);
	}

	void setPosition(const sf::Vector2f pos)
	{
		circle.setPosition(pos);
		text.setPosition(pos);
	}
	void draw(sf::RenderWindow &window)
	{
		window.draw(circle);
		window.draw(text);
	}

	void deconstruct()
	{
		symbol = 0;
		father = -1;
		leaf[0] = -1;
		leaf[1] = -1;
	}
};
class Tree
{
private:

	void _addToTree(Operator op, vector<Operator> &operatorStack, vector<Operand> &operandStack)
	{
		int currentNode = nodes.size(); //the index of the new node

		if (op.arity == 2)
		{
			Operand aux1 = operandStack[operandStack.size() - 2];
			Operand aux2 = operandStack.back();
			operandStack.pop_back();
			operandStack.pop_back();

			//creating nodes and linking
			if (aux1.isTreeHandle)
			{
				if (aux2.isTreeHandle) //both
				{
					//tree linkers
					nodes.push_back(Node(operatorStack.back().symbol, -1, aux1.handle, aux2.handle));
					nodes[aux1.handle].father = currentNode;
					nodes[aux2.handle].father = currentNode;
				}
				else //first is tree
				{
					nodes.push_back(Node(operatorStack.back().symbol, -1, aux1.handle, currentNode + 1));
					nodes[aux1.handle].father = currentNode;
					nodes.push_back(Node(aux2.symbol, currentNode, -1, -1));
				}
			}
			else
			{
				if (aux2.isTreeHandle) //second is tree
				{
					nodes.push_back(Node(operatorStack.back().symbol, -1, currentNode + 1, aux2.handle));
					nodes.push_back(Node(aux1.symbol, currentNode, -1, -1));
					nodes[aux2.handle].father = currentNode;
				}
				else //none
				{
					nodes.push_back(Node(operatorStack.back().symbol, -1, currentNode + 1, currentNode + 2));
					nodes.push_back(Node(aux1.symbol, currentNode, -1, -1));
					nodes.push_back(Node(aux2.symbol, currentNode, -1, -1));
				}
			}
			operandStack.push_back(Operand(currentNode));
			cout << "Pushed: " << nodes[currentNode].symbol << " into the operand stack\n";
		}
		else if (op.arity == 1)
		{
			Operand aux1 = operandStack.back();
			operandStack.pop_back();

			//creating nodes and linking
			if (aux1.isTreeHandle)
			{
				nodes.push_back(Node(operatorStack.back().symbol, -1, aux1.handle, -1));
				nodes[aux1.handle].father = currentNode;
			}
			else
			{
				nodes.push_back(Node(operatorStack.back().symbol, -1, currentNode + 1, -1));
				nodes.push_back(Node(aux1.symbol, currentNode, -1, -1));
			}
			operandStack.push_back(Operand(currentNode));
			cout << "Pushed: " << nodes[currentNode].symbol << " into the operand stack\n";
		}

		operatorStack.pop_back();
	}
	void _getSubtreeLeaves(int pos, int &leaves)
	{
		if (nodes[pos].leaf[0] != -1)
		{
			_getSubtreeLeaves(nodes[pos].leaf[0], leaves);
			if (nodes[pos].leaf[1] != -1)
				_getSubtreeLeaves(nodes[pos].leaf[1], leaves);
		}
		else
		{
			leaves++;
		}
	}
	void _recTreeString(char treeString[256][256], int pos, int x, int y)
	{
		if (x >= 0 && y >= 0 && x < 254 && y < 254)
		{
			treeString[y][x] = nodes[pos].symbol;
			if (nodes[pos].leaf[0] != -1)
			{
				if (nodes[pos].leaf[1] != -1)
				{
					int depth = -1;
					_getSubtreeLeaves(pos, depth);

					for (int i = 0; i < depth; i++)
					{
						treeString[y + i + 1][x - i - 1] = '/';
						treeString[y + i + 1][x + i + 1] = '\\';
					}
					_recTreeString(treeString, nodes[pos].leaf[0], x - depth - 1, y + depth + 1);
					_recTreeString(treeString, nodes[pos].leaf[1], x + depth + 1, y + depth + 1);
				}
				else
				{
					int depth = -1;
					_getSubtreeLeaves(pos, depth);

					for (int i = 0; i < depth + 1; i++)
						treeString[y + i + 1][x - i - 1] = '/';
					_recTreeString(treeString, nodes[pos].leaf[0], x - depth - 2, y + depth + 2);
				}
			}
		}
	}
	void _recSetPos(int pos, int x, int y)
	{
		nodes[pos].setPosition(sf::Vector2f(x * 20.0, y * 20.0));
		if (nodes[pos].leaf[0] != -1)
		{
			int depth = 0;
			_getSubtreeLeaves(pos, depth);
			_recSetPos(nodes[pos].leaf[0], x - 2 * depth - 1, y + 2 * depth + 1);

			if(nodes[pos].leaf[1] != -1)
				_recSetPos(nodes[pos].leaf[1], x + 2 * depth + 1, y + 2 * depth + 1);
		}
	}

public:
	int root = 0;
	vector<Node> nodes;
	vector<Operator> operatorSet;
	vector<Operand> operandSet;
	StackArchive archive;

	void setDefaultSet()
	{
		operatorSet.push_back(Operator('+', 1, 2)); //ADD
		operatorSet.push_back(Operator('-', 1, 2)); //SUB
		operatorSet.push_back(Operator('*', 2, 2)); //MLT
		operatorSet.push_back(Operator('/', 2, 2)); //DIV
		operatorSet.push_back(Operator('-', 3, 1)); //OPS
	}
	void setOperatorSet(const vector<Operator> opSet)
	{
		operatorSet = opSet;
	}

	void clear()
	{
		nodes.clear();
		archive.clear();
		operatorSet.clear();
		operandSet.clear();
		root = 0;
	}

	int isOperator(char c, const int arity)
	{
		int size = operatorSet.size();
		for (int i = 0; i < size; ++i)
			if (c == operatorSet[i].symbol && arity == operatorSet[i].arity)
				return i;
		return -1;
	}
	bool isOperator(char c)
	{ //find if operator of any arity
		int size = operatorSet.size();
		for (int i = 0; i < size; ++i)
			if (c == operatorSet[i].symbol)
				return true;
		return false;
	}
	int isOperand(char c)
	{
		int size = operandSet.size();
		for (int i = 0; i < size; ++i)
			if (c == operandSet[i].symbol)
				return i;
		return -1;
	}

	bool construct(const string s)
	{
		//clearings
		clear();

		if (!operatorSet.size()) //use default set when no set is given
			setDefaultSet();

		vector<Operator> operatorStack;
		vector<Operand> operandStack;
		int parenthesisLevel = 0, nrOperators = operatorSet.size();

		int stringSize = s.size();
		for (int i = 0; i < stringSize; ++i)
		{//search known operators
			if (s[i] == '(')
			{
				++parenthesisLevel;
				continue;
			}
			else if (s[i] == ')')
			{
				--parenthesisLevel;
				continue;
			}

			int  j;

			if (isOperator(s[i], 1) != -1 && isOperator(s[i], 2) != -1)
			{
				if (i == 0 || isOperator(s[i - 1]) || s[i - 1] == '(')
					j = isOperator(s[i], 1);
				else
					j = isOperator(s[i], 2);
			}
			else
			{
				j = isOperator(s[i], 2);
				if (j == -1)
					j = isOperator(s[i], 1);
			}

			if (j != -1) //operator found
			{
				while (operatorStack.size() && operatorStack.back().precedance > operatorSet[j].precedance + parenthesisLevel * nrOperators) //operatorStack.top().precedance > operatorList[0].precedance
				{
					cout << "Solving precedence on: " << operatorStack.back().symbol << '\n';
					_addToTree(operatorStack.back(), operatorStack, operandStack);

				}

				//after solving precedencies add operator
				operatorStack.push_back(operatorSet[j]);
				operatorStack.back().precedance += parenthesisLevel * nrOperators;

				cout << "Pushed: " << operatorSet[j].symbol << " prec: " << operatorStack.back().precedance << " into the operator stack\n";
			}
			else //operands
			{
				//search known operands
				int j = isOperand(s[i]);
				if (j != -1)
				{
					operandStack.push_back(operandSet[j]);
					cout << "Pushed: " << operandSet[j].symbol << " into the operand stack\n";
				}
				else //add new operand
				{
					operandSet.push_back(Operand(s[i], 0));
					operandStack.push_back(operandSet[operandSet.size() - 1]);
					cout << "Pushed: " << operandSet[operandSet.size() - 1].symbol << " into the operand stack\n";
				}
			}

			printStacks(operatorStack, operandStack);
			archive.push(operatorStack);
			archive.push(operandStack);
		}

		cout << "Leftovers:\n";
		//solving leftovers

		while (operatorStack.size() && operandStack.size())
		{
			_addToTree(operatorStack.back(), operatorStack, operandStack);
			printStacks(operatorStack, operandStack);
			archive.push(operatorStack);
			archive.push(operandStack);
		}

		if (operatorStack.size())
			cout << "The given string is not an expression: Operator stack not empty\n";
		else if (operandStack.size() > 1)
			cout << "The given string is not an expression: Operand stack not empty\n";
		else if (parenthesisLevel != 0)
			cout << "The given string is not an expression: Unpaired parenthesis\n";
		else
		{
			cout << "Success!\n";
			root = operandStack.front().handle;
		}

		//clearing stacks
		operandStack.clear();
		operatorStack.clear();
	}

	void printTreeData()
	{
		for (int i = 0; i < nodes.size(); ++i)
			cout << "Node " << i << ": symbol " << nodes[i].symbol << ", father: " << nodes[i].father << ", leaf 1: " << nodes[i].leaf[0] << ", leaf 2: " << nodes[i].leaf[1] << '\n';
	}
	void printTree()
	{
		char treeString[256][256];
		for (int i = 0; i < 256; i++)
		{ //create empty char matrix
			for (int j = 0; j < 255; j++)
				treeString[i][j] = ' ';
			treeString[i][255] = 0;
		}

		//start as close to 0 0 as posible 
		int allLeaves = 0;
		_getSubtreeLeaves(root, allLeaves);
		_recTreeString(treeString, root, allLeaves * 4 + 1, 0);
		//delete useless spaces
		for (int i = 0; i < 256; i++)
		{
			for (int j = 254; j >= 0; j--)
				if (treeString[i][j] != ' ')
				{
					treeString[i][j + 1] = 0;
					break;
				}
			treeString[i][255] = 0;
		}

		cout << '\n';
		for (int i = 0; i < 256; i++)
		{ //print matrix
			bool ok = false;
			for (int j = 0; j < 255; j++)
				if (treeString[i][j] != ' ')
				{
					ok = true;
					break;
				}
			if (!ok)
				break;
			cout << treeString[i] << '\n';
		}
		cout << '\n';
	}
	void printTree(const int root)
	{
		char treeString[256][256];
		for (int i = 0; i < 256; i++)
		{ //create empty char matrix
			for (int j = 0; j < 255; j++)
				treeString[i][j] = ' ';
			treeString[i][255] = 0;
		}

		//start as close to 0 0 as posible 
		int allLeaves = 0;
		_getSubtreeLeaves(root, allLeaves);
		_recTreeString(treeString, root, allLeaves * 4 + 1, 0);
		//delete useless spaces
		for (int i = 0; i < 256; i++)
		{
			for (int j = 254; j >= 0; j--)
				if (treeString[i][j] != ' ')
				{
					treeString[i][j + 1] = 0;
					break;
				}
			treeString[i][255] = 0;
		}

		cout << '\n';
		for (int i = 0; i < 256; i++)
		{ //print matrix
			bool ok = false;
			for (int j = 0; j < 255; j++)
				if (treeString[i][j] != ' ')
				{
					ok = true;
					break;
				}
			if (!ok)
				break;
			cout << treeString[i] << '\n';
		}
		cout << '\n';
	}
	void printStacks(vector<Operator> operatorStack, vector<Operand> operandStack)
	{
		cout << "Operator Stack: ";
		for (int i = 0; i < operatorStack.size(); i++)
			cout << '[' << operatorStack[i].symbol << "] ";
		cout << '\n';
		cout << "Operand Stack: ";
		for (int i = 0; i < operandStack.size(); i++)
			cout << '[' << operandStack[i].symbol << "] ";
		cout << '\n';
	}
};

int main()
{
	sf::RenderWindow window(sf::VideoMode(WINDOW_SIZE_X, WINDOW_SIZE_Y), "FLAT");
	window.setFramerateLimit(60);

	sf::RectangleShape mainBackground(sf::Vector2f(WINDOW_SIZE_X, WINDOW_SIZE_Y));
	mainBackground.setFillColor(sf::Color(0, 50, 20, 255));

	sf::Mouse mouse;
	
	int selectedTB = -1, nrTB = 2;
	InputBox textBox[2];
	for (int i = 0; i < nrTB; i++)
		textBox[i].construct(sf::Vector2f(20.0, i*55), sf::Vector2f(200.0, 50.0), sf::Color(100, 100, 100, 255), sf::Color(200, 200, 200, 255), "Type Here");

	Button b; b.construct(sf::Vector2f(320.0, 55.0), sf::Vector2i(22, 20), sf::Vector2f(1.0, 1.0), "iconx.png", true);
	//SquareBox sb(sf::Vector2f(200.0, 200.0), sf::Vector2f(50.0, 50.0), sf::Color(255, 120, 0, 255), "@1");
	
	Tree tree; int archiveViewStep = 0;
	bool st_STEPVIEW = false;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::MouseButtonReleased)
			{
				if (b.containsPoint(mouse.getPosition(window)))
				{
					b.onRelease();
					if (textBox[0].textStr.size())
					{
						tree.construct(textBox[0].textStr);
						st_STEPVIEW = true;
						archiveViewStep = 0;
					}
				}
			}
			if (event.type == sf::Event::MouseButtonPressed)
			{
				bool clickedTB = false;
				for (int i = 0; i < nrTB; i++)
					if (textBox[i].containsPoint(mouse.getPosition(window)))
					{
						if (selectedTB != -1)
							textBox[selectedTB].isSelected = false;
						selectedTB = i;
						textBox[selectedTB].isSelected = true;
						clickedTB = true;
						break;
					}
				if (!clickedTB && selectedTB != -1)
				{
					textBox[selectedTB].isSelected = false;
					selectedTB = -1;
				}

				if (b.containsPoint(mouse.getPosition(window)))
					b.onPress();
			}
			else if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == sf::Keyboard::Right)
				{
					if (selectedTB != -1)
						textBox[selectedTB].moveCursor(1);
				}
				else if (event.key.code == sf::Keyboard::Left)
				{
					if (selectedTB != -1)
						textBox[selectedTB].moveCursor(-1);
				}
				else if (st_STEPVIEW && event.key.code == sf::Keyboard::K)
				{
					++archiveViewStep;
					if (archiveViewStep >= tree.archive.size / 2) //reached end
					{
						archiveViewStep = 0;
					}
				}
			}
			else if (event.type == sf::Event::TextEntered)
			{
				if (selectedTB != -1)
				{
					if (textBox[selectedTB].modifyString(event.text.unicode)) //enter
						selectedTB = -1;
				}
			}
			else if (event.type == sf::Event::Closed)
				window.close();
		}
		window.clear();
		window.draw(mainBackground);
		for (int i = 0; i < nrTB; i++)
			textBox[i].draw(window);
		b.draw(window);
		if (st_STEPVIEW)
			tree.archive.draw(sf::Vector2f(200.0, 200.0), sf::Vector2f(300.0, 200.0), archiveViewStep, window);
		window.display();
	}

	return 0;
}