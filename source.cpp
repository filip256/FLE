#include <SFML/Graphics.hpp>
#include <Windows.h>
#include <iostream>

#define WINDOW_SIZE_X 1800.0
#define WINDOW_SIZE_Y 800.0

using namespace std;

HWND hwnd;
HMENU hmenu;
HANDLE hOutput;
HANDLE hInput;
HDC cHdc;
CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
CONSOLE_CURSOR_INFO cursorInfo;

void drawText(const string text, const int fg, const int bg)
{
	SetConsoleTextAttribute(hOutput, fg + bg * 16);
	cout << text;
	SetConsoleTextAttribute(hOutput, 15);
}
void copyToClipboard(string s)
{
	//cast to char*
	int n = s.size();
	char* c_str = &s[0];

	const size_t len = s.size() + 1;
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
	memcpy(GlobalLock(hMem), c_str, len);
	GlobalUnlock(hMem);
	OpenClipboard(0);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
}
string getFromClipboard()
{
	OpenClipboard(0);
	HANDLE hData = GetClipboardData(CF_TEXT);
	char *p = static_cast<char*>(GlobalLock(hData));
	GlobalUnlock(hData);
	CloseClipboard();

	return string(p);
}

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
class ColorBox
{
public:
	string textStr;

	sf::Vector2f pos, size;
	sf::Color backgroundCol;
	sf::RectangleShape background;

	void construct(const sf::Vector2f p, const sf::Vector2f s, const sf::Color bgCol)
	{
		pos = p;
		size = s;
		backgroundCol = bgCol;

		background.setSize(s);
		background.setPosition(p);
		background.setFillColor(bgCol);
	}

	void draw(sf::RenderWindow &window)
	{
		window.draw(background);
	}
};
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
		text.setCharacterSize(24);
		text.setPosition(p.x + 3, p.y + (s.y - text.getGlobalBounds().height) / 2.0);
	}
	void setText(const string s)
	{
		textStr = s;
		text.setString(textStr);
		text.setPosition(pos.x + 3, pos.y + (size.y - text.getGlobalBounds().height) / 2.0);
	}

	bool containsPoint(const sf::Vector2i p)
	{
		if (p.x >= pos.x && p.y >= pos.y && p.x <= pos.x + background.getGlobalBounds().width && p.y <= pos.y + background.getGlobalBounds().height)
			return true;
		return false;
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

		text.setCharacterSize(24);
		text.setPosition(p.x + 3, p.y + (s.y - text.getGlobalBounds().height) / 2.0);


		cursor.setSize(sf::Vector2f(2.0, 24.0));
		cursor.setFillColor(sf::Color::White);
		cursor.setPosition(p.x + 3, p.y + (s.y - 14) / 2.0);
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
			cursor.setPosition(pos.x + cursorPos * 14, cursor.getPosition().y);
			blink = 5;
		}
	}
	bool modifyString(const char &c)
	{
		//int a = text.getGlobalBounds().width;
		if (c == 8 && cursorPos)
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

			//while (text.getGlobalBounds().width > size.x - 20 && text.getCharacterSize() >= 16)
			//	text.setCharacterSize(text.getCharacterSize() - 2);
		}
		else
		{
			text.setString(defTextStr);
			text.setFillColor(sf::Color(backgroundCol.r + 50, backgroundCol.g + 50, backgroundCol.b + 50, backgroundCol.a));
			text.setStyle(sf::Text::Italic);
			cursor.setPosition(pos.x + 3, pos.y + (size.y - 14) / 2.0);
		}

		//cout << a - text.getGlobalBounds().width<<'\n';

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
			if (blink < 15)
				window.draw(cursor);

			if (blink < 30)
				++blink;
			else
				blink = 0;
		}
	}
};
class Button
{
public:
	bool isAnimated = false, isSwitch = false;
	int clicked = 0;
	bool isOn = true;

	sf::Vector2f pos;
	sf::Vector2i size;
	sf::Texture tx;
	sf::Sprite sp;

	void construct(const sf::Vector2f p, const sf::Vector2i s, const sf::Vector2f scale, const string location, const bool anim, const bool swit)
	{
		if (!tx.loadFromFile(location))
		{
			drawText("Couldn't open texture file: ", 4, 0); drawText(location, 8, 0); drawText("\n", 8, 0);
		}
		else
		{
			isAnimated = anim;
			isSwitch = swit;
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
		if (isAnimated || (isSwitch && isOn))
			sp.setTextureRect(sf::IntRect(size.x, 0, size.x, size.y));
		else if(!isSwitch)
			sp.setColor(sf::Color(200, 200, 200, 200));
	}
	void onRelease()
	{
		isOn = !isOn;
		if (isAnimated || (isSwitch && isOn))
			sp.setTextureRect(sf::IntRect(0, 0, size.x, size.y));
		else if(!isSwitch)
			sp.setColor(sf::Color(255, 255, 255, 255));
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
	SquareBox(const sf::Vector2f p, const sf::Vector2f s, const sf::Color col, const sf::Color outCol, const string txt)
	{
		pos = p;
		size = s;
		color = col;
		textStr = txt;

		square.setPosition(pos);
		square.setFillColor(color);
		square.setSize(size);
		square.setOutlineThickness(2.0);
		square.setOutlineColor(outCol);

		font.loadFromFile("dosfont.ttf");

		text.setFont(font);
		text.setString(textStr);
		text.setFillColor(sf::Color::Black);

		text.setCharacterSize(24);
		while (text.getGlobalBounds().width > size.x - 4)
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

	bool containsPoint(const sf::Vector2i p)
	{
		if (p.x >= pos.x && p.y >= pos.y && p.x <= pos.x + size.x && p.y <= pos.y + size.y)
			return true;
		return false;
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
			boxList.push_back(SquareBox(sf::Vector2f(0.0, 0.0), sf::Vector2f(40.0, 40.0), sf::Color::Cyan, sf::Color(0, 175, 175, 255), string(1, stack[i].symbol)));
		}
	}
	void setStack(const vector<Operand> &stack)
	{
		boxList.clear();
		const int size = stack.size();
		for (int i = 0; i < size; ++i)
		{
			if (!stack[i].isTreeHandle)
				boxList.push_back(SquareBox(sf::Vector2f(0.0, 0.0), sf::Vector2f(40.0, 40.0), sf::Color::Yellow, sf::Color(175, 175, 0, 255), string(1, stack[i].symbol)));
			else
				boxList.push_back(SquareBox(sf::Vector2f(0.0, 0.0), sf::Vector2f(40.0, 40.0), sf::Color::Yellow, sf::Color(175, 175, 0, 255), "@" + to_string(stack[i].handle)));
		}
	}

	void setPosition(const sf::Vector2f pos)
	{
		const int size = boxList.size();
		for (int i = 0; i < size; ++i)
			boxList[i].setPosition(sf::Vector2f(pos.x + i * (boxList[i].size.x + boxList[i].square.getOutlineThickness() + 10), pos.y));
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
	GraphicStack gStack;

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
	void draw(const sf::Vector2f pos1, const sf::Vector2f pos2, const int step, sf::RenderWindow &window)
	{
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
	bool highlight = false;

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
		text.setCharacterSize(24);
		text.setFillColor(sf::Color::Black);


		circle.setRadius(20.0);
		circle.setPointCount(20);
		circle.setOutlineThickness(2.0);

		if (leaf[0] != -1)
		{
			circle.setFillColor(sf::Color(0, 255, 255, 255));
			circle.setOutlineColor(sf::Color(0, 175, 175, 255));
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
	void setHighlight(const bool h)
	{
		highlight = h;
		if (!highlight)
		{
			if (leaf[0] != -1)
			{
				circle.setFillColor(sf::Color(0, 255, 255, 255));
				circle.setOutlineColor(sf::Color(0, 175, 175, 255));
			}
			else
			{
				circle.setFillColor(sf::Color(255, 255, 0, 255));
				circle.setOutlineColor(sf::Color(175, 175, 0, 255));
			}
			text.setFillColor(sf::Color::Black);
			circle.setOutlineThickness(2.0);
		}
		else
		{
			if (leaf[0] != -1)
			{
				circle.setFillColor(sf::Color(150, 255, 255, 255));
				circle.setOutlineColor(sf::Color(150, 175, 175, 255));
			}
			else
			{
				circle.setFillColor(sf::Color(255, 255, 150, 255));
				circle.setOutlineColor(sf::Color(175, 175, 70, 255));
			}
			text.setFillColor(sf::Color(100, 100, 100, 255));
			circle.setOutlineThickness(3.0);
		}
	}
	void draw(sf::RenderWindow &window)
	{
		text.setFont(font); //SFML beeing lazy
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

			//printings
			drawText("Popped '", 8, 0);
			if (aux1.isTreeHandle) drawText("@" + to_string(aux1.handle), 6, 0);
			else drawText(string(1, aux1.symbol), 6, 0);
			drawText("' from ", 8, 0); drawText("OPERAND STACK\n", 6, 0);
			drawText("Popped '", 8, 0);
			if (aux2.isTreeHandle) drawText("@" + to_string(aux2.handle), 6, 0);
			else drawText(string(1, aux2.symbol), 6, 0);
			drawText("' from ", 8, 0); drawText("OPERAND STACK\n", 6, 0);
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

			//printings
			drawText("Popped '", 8, 0);
			if (aux1.isTreeHandle) drawText("@" + to_string(aux1.handle), 6, 0);
			else drawText(string(1, aux1.symbol), 6, 0);
			drawText("' from ", 8, 0); drawText("OPERAND STACK\n", 6, 0);
		}

		drawText("Popped '", 8, 0); drawText(string(1, operatorStack.back().symbol), 3, 0); drawText("' from ", 8, 0); drawText("OPERATOR STACK\n", 3, 0);
		drawText("'", 8, 0); drawText("@" + to_string(currentNode), 6, 0); drawText("' pushed in ", 8, 0); drawText("OPERAND STACK\n", 6, 0);
		operatorStack.pop_back();
	}
	void _getSubtreeLeaves(int pos, int &leaves)
	{
		if (nodes[pos].leaf[0] != -1)
		{
			_getSubtreeLeaves(nodes[pos].leaf[0], leaves);
			if (nodes[pos].leaf[1] != -1)
				_getSubtreeLeaves(nodes[pos].leaf[1], leaves);
			else
				leaves++; //trick for correct printing complex trees
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
		nodes[pos].setPosition(sf::Vector2f(x * 15.0, y * 15.0));
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
		operatorSet.push_back(Operator('-', 4, 1)); //OPS
		operatorSet.push_back(Operator('^', 3, 2)); //PWR
		operatorSet.push_back(Operator('%', 3, 2)); //MOD

		operatorSet.push_back(Operator('!', 5, 1)); //NEG
		operatorSet.push_back(Operator('&', 4, 2)); //AND
		operatorSet.push_back(Operator('|', 3, 2)); //OR
		operatorSet.push_back(Operator('$', 3, 2)); //XOR
		operatorSet.push_back(Operator('~', 2, 2)); //IMP
		operatorSet.push_back(Operator('=', 1, 2)); //EQU

		operatorSet.push_back(Operator('>', 6, 2)); //GTR
		operatorSet.push_back(Operator('<', 6, 2)); //LSS

	}
	void setOperatorSet(const vector<Operator> opSet)
	{
		operatorSet = opSet;
	}
	void setHighlight(const bool h)
	{
		int size = nodes.size();
		for (int i = 0; i < size; ++i)
			nodes[i].setHighlight(h);
	}
	void setHighlight(const bool h, const int pos)
	{
		nodes[pos].setHighlight(h);
		if (nodes[pos].leaf[0] != -1)
		{
			if (nodes[pos].leaf[1] != -1)
				setHighlight(h, nodes[pos].leaf[1]);
			setHighlight(h, nodes[pos].leaf[0]);
		}
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

	int construct(const string s)
	{
		//clearings
		clear();

		if (!operatorSet.size()) //use default set when no set is given
			setDefaultSet();

		vector<Operator> operatorStack;
		vector<Operand> operandStack;
		int parenthesisLevel = 0, nrOperators = operatorSet.size();
		bool secondOperand = false;
		int stringSize = s.size();
		for (int i = 0; i < stringSize; ++i)
		{//search known operators
			if (s[i] == '(')
			{
				++parenthesisLevel;
				drawText("'", 8, 0); drawText("(", 7, 0); drawText("' predecence level set to ", 8, 0); drawText(to_string(parenthesisLevel), 7, 0); drawText("\n", 15, 0);
				continue;
			}
			if (s[i] == ')')
			{
				--parenthesisLevel;
				drawText("'", 8, 0); drawText(")", 7, 0); drawText("' predecence level set to ", 8, 0); drawText(to_string(parenthesisLevel), 7, 0); drawText("\n", 15, 0);
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
				while (operatorStack.size() && (
						(operatorSet[j].arity == 2 && operatorStack.back().precedance >= operatorSet[j].precedance + parenthesisLevel * nrOperators) ||
						(operatorSet[j].arity == 1 && operatorStack.back().precedance > operatorSet[j].precedance + parenthesisLevel * nrOperators)
											   )) //read both unary and binary in their correct "human-like" order 
				{
					if (operandStack.size() >= operatorSet[j].arity - 1)
					{
						drawText("'", 8, 0); drawText(string(1, operatorSet[j].symbol), 3, 0); drawText("' solved precedence issue with: '", 8, 0); drawText(string(1, operatorStack.back().symbol), 3, 0); drawText("'\n", 8, 0);
						_addToTree(operatorStack.back(), operatorStack, operandStack);
					}
					else
						return 4; //missing operand
				}

				//after solving precedencies add operator
				operatorStack.push_back(operatorSet[j]);
				operatorStack.back().precedance += parenthesisLevel * nrOperators;

				secondOperand = false;

				drawText("'", 8, 0); drawText(string(1, operatorSet[j].symbol), 3, 0); drawText("' pushed in ", 8, 0); drawText("OPERATOR STACK\n", 3, 0);
			}
			else //operands
			{
				if (secondOperand)
					return 5; //2 operands in a row
				//search known operands
				int j = isOperand(s[i]);
				if (j != -1)
				{
					operandStack.push_back(operandSet[j]);
					drawText("'", 8, 0); drawText(string(1, operandSet[j].symbol), 6, 0); drawText("' pushed in ", 8, 0); drawText("OPERAND STACK\n", 6, 0);
				}
				else //add new operand
				{
					operandSet.push_back(Operand(s[i], 0));
					operandStack.push_back(operandSet[operandSet.size() - 1]);
					drawText("'", 8, 0); drawText(string(1, operandSet[operandSet.size() - 1].symbol), 6, 0); drawText("' pushed in ", 8, 0); drawText("OPERAND STACK\n", 6, 0);
				}
				secondOperand = true;
			}

			printStacks(operatorStack, operandStack);
			archive.push(operatorStack);
			archive.push(operandStack);
		}

		//solving leftovers
		while (operatorStack.size() && operandStack.size())
		{
			if (operandStack.size() >= operatorStack.back().arity)
			{
				_addToTree(operatorStack.back(), operatorStack, operandStack);
				printStacks(operatorStack, operandStack);
				archive.push(operatorStack);
				archive.push(operandStack);
			}
			else
				return 4;
		}

		//errors
		if (operatorStack.size())
			return 1; //Operator stack not empty;
		if (operandStack.size() > 1)
			return 2; //Operand stack not empty
		if (parenthesisLevel != 0)
			return 3; //Unpaired parenthesis

		if(operandStack.front().isTreeHandle)
			root = operandStack.front().handle;
		else //only one character
		{
			nodes.push_back(Node(operandStack.front().symbol, -1, -1, -1));
			root = nodes.size() - 1; //should be 0 always but just to be safe
		}

		//for display
		_recSetPos(root, 93, 4);
		//clearing stacks
		operandStack.clear();
		operatorStack.clear();
		return 0;
	}

	string getPrefixNotation(const int pos)
	{
		if (nodes[pos].leaf[0] != -1)
		{
			if (nodes[pos].leaf[1] != -1)
				return nodes[pos].symbol + getPrefixNotation(nodes[pos].leaf[0]) + getPrefixNotation(nodes[pos].leaf[1]);
			return nodes[pos].symbol + getPrefixNotation(nodes[pos].leaf[0]);
		}
		return string(1, nodes[pos].symbol);	
	}
	string getPrefixNotation_o(const int pos)
	{
		string str = "";
		if (nodes[pos].leaf[0] != -1)
		{
			if (nodes[pos].leaf[1] != -1)
				str += nodes[pos].symbol + getPrefixNotation_o(nodes[pos].leaf[0]) + getPrefixNotation_o(nodes[pos].leaf[1]);
			else
				str += nodes[pos].symbol + getPrefixNotation_o(nodes[pos].leaf[0]);
		}
		else
			str += string(1, nodes[pos].symbol);
		cout << nodes[pos].symbol << " : " << str << '\n';
		return str;
	}
	int getValue(const int pos)
	{
		if (nodes[pos].symbol >= '0' && nodes[pos].symbol <= '9')
			return nodes[pos].symbol - '0';
		else if (nodes[pos].symbol == '&')
			return getValue(nodes[pos].leaf[0]) & getValue(nodes[pos].leaf[1]);
		else if (nodes[pos].symbol == '|')
			return getValue(nodes[pos].leaf[0]) | getValue(nodes[pos].leaf[1]);
		else if (nodes[pos].symbol == '!')
			return ~getValue(nodes[pos].leaf[0]);
		else if (nodes[pos].symbol == '$')
			return getValue(nodes[pos].leaf[0]) ^ getValue(nodes[pos].leaf[1]);
		else if (nodes[pos].symbol == '~')
		{
			int a = getValue(nodes[pos].leaf[0]), b = getValue(nodes[pos].leaf[1]);
			if (a == 0 || a == b)
				return 1;
			return 0;
		}
		else if (nodes[pos].symbol == '=')
		{
			if (getValue(nodes[pos].leaf[0]) == getValue(nodes[pos].leaf[1]))
				return 1;
			return 0;
		}
		else if (nodes[pos].symbol == '+')
			return getValue(nodes[pos].leaf[0]) + getValue(nodes[pos].leaf[1]);
		else if (nodes[pos].symbol == '*')
			return getValue(nodes[pos].leaf[0]) * getValue(nodes[pos].leaf[1]);
		else if (nodes[pos].symbol == '/')
			return getValue(nodes[pos].leaf[0]) / getValue(nodes[pos].leaf[1]);
		else if (nodes[pos].symbol == '^')
			return pow(getValue(nodes[pos].leaf[0]), getValue(nodes[pos].leaf[1]));
		else if (nodes[pos].symbol == '%')
			return getValue(nodes[pos].leaf[0]) % getValue(nodes[pos].leaf[1]);
		else if (nodes[pos].symbol == '-')
		{
			if (nodes[pos].leaf[1] != -1)
				return getValue(nodes[pos].leaf[0]) - getValue(nodes[pos].leaf[1]);
			else
				return -getValue(nodes[pos].leaf[0]);
		}
		else if (nodes[pos].symbol == '>')
			return getValue(nodes[pos].leaf[0]) > getValue(nodes[pos].leaf[1]);
		else if (nodes[pos].symbol == '<')
			return getValue(nodes[pos].leaf[0]) < getValue(nodes[pos].leaf[1]);
		else
			return 0;
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

			string aux = "";
			for (int j = 0; j < 255; j++)
			{
				if (treeString[i][j] == ' ')
					aux += " ";
				else if(treeString[i][j] == '/' || treeString[i][j] == '\\')
				{
					aux += treeString[i][j];
					drawText(aux, 8, 0);
					aux = "";
				}
				else if(isOperator(treeString[i][j]))
				{
					aux += treeString[i][j];
					drawText(aux, 3, 0);
					aux = "";
				}
				else
				{
					aux += treeString[i][j];
					drawText(aux, 6, 0);
					aux = "";
				}
			}
			cout << '\n';
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

			string aux = "";
			for (int j = 0; j < 255; j++)
			{
				if (treeString[i][j] == ' ')
					aux += " ";
				else if (treeString[i][j] == '/' || treeString[i][j] == '\\')
				{
					aux += treeString[i][j];
					drawText(aux, 8, 0);
					aux = "";
				}
				else if (isOperator(treeString[i][j]))
				{
					aux += treeString[i][j];
					drawText(aux, 3, 0);
					aux = "";
				}
				else
				{
					aux += treeString[i][j];
					drawText(aux, 6, 0);
					aux = "";
				}
			}
			cout << '\n';
		}
		cout << '\n';
	}
	void printStacks(vector<Operator> operatorStack, vector<Operand> operandStack)
	{
		/*
		cout << "Operator Stack: ";
		for (int i = 0; i < operatorStack.size(); i++)
			cout << '[' << operatorStack[i].symbol << "] ";
		cout << '\n';
		cout << "Operand Stack: ";
		for (int i = 0; i < operandStack.size(); i++)
			cout << '[' << operandStack[i].symbol << "] ";
		cout << '\n';
		*/
	}

	void move(const sf::Vector2f pos)
	{
		const int size = nodes.size();
		for (int i = 0; i < size; i++)
			nodes[i].setPosition(sf::Vector2f(nodes[i].circle.getPosition().x + pos.x, nodes[i].circle.getPosition().y + pos.y));
	}
	void draw(sf::RenderWindow &window)
	{
		sf::Vertex line[2];
		int size = nodes.size();
		for (int i = 0; i < size; i++)
		{
			if (nodes[i].leaf[0] != -1)
			{
				line[0].position = nodes[i].circle.getPosition();
				line[1].position = nodes[nodes[i].leaf[0]].circle.getPosition();
				if (nodes[i].highlight && nodes[nodes[i].leaf[0]].highlight)
				{
					line[0].color = sf::Color(255, 255, 255, 255);
					line[1].color = sf::Color(255, 255, 255, 255);
				}
				else
				{
					line[0].color = sf::Color(155, 155, 155, 255);
					line[1].color = sf::Color(155, 155, 155, 255);
				}
				window.draw(line, 2, sf::Lines);
			}
			if (nodes[i].leaf[1] != -1)
			{
				line[0].position = nodes[i].circle.getPosition();
				line[1].position = nodes[nodes[i].leaf[1]].circle.getPosition();
				if (nodes[i].highlight && nodes[nodes[i].leaf[0]].highlight)
				{
					line[0].color = sf::Color(255, 255, 255, 255);
					line[1].color = sf::Color(255, 255, 255, 255);
				}
				else
				{
					line[0].color = sf::Color(155, 155, 155, 255);
					line[1].color = sf::Color(155, 155, 155, 255);
				}
				window.draw(line, 2, sf::Lines);
			}
		}
		for (int i = 0; i < size; i++)
		{
			//check if inside tree window
			if(nodes[i].circle.getPosition().x - nodes[i].circle.getRadius() < WINDOW_SIZE_X && nodes[i].circle.getPosition().x + nodes[i].circle.getRadius() > WINDOW_SIZE_X - WINDOW_SIZE_Y && nodes[i].circle.getPosition().y - nodes[i].circle.getRadius() < WINDOW_SIZE_Y && nodes[i].circle.getPosition().y + nodes[i].circle.getRadius() > 0)
				nodes[i].draw(window);
		}
	}
};

void _setup(InputBox inBox[], Button button[], TextBox txBox[], ColorBox clBox[])
{
	hwnd = GetConsoleWindow();
	hmenu = GetSystemMenu(hwnd, FALSE);
	hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	hInput = GetStdHandle(STD_INPUT_HANDLE);
	cHdc = GetDC(hwnd);
	bufferInfo;
	GetConsoleScreenBufferInfo(hOutput, &bufferInfo);
	GetConsoleCursorInfo(hOutput, &cursorInfo);

	//remove exit button from console
	EnableMenuItem(hmenu, SC_CLOSE, MF_GRAYED);

	//Setting a font
	CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize = sizeof(cfi);
	cfi.nFont = 0;
	cfi.dwFontSize.X = 10;                   // Width of each character in the font
	cfi.dwFontSize.Y = 16;                  // Height
	cfi.FontFamily = FF_DONTCARE;
	cfi.FontWeight = FW_NORMAL;
	wcscpy_s(cfi.FaceName, L"Consolas"); // Choose your font
	SetCurrentConsoleFontEx(hOutput, FALSE, &cfi);

	
	cursorInfo.bVisible = false;
	SetConsoleCursorInfo(hOutput, &cursorInfo);

	SetConsoleTitle(TEXT("FLEE Log Console"));

	//UI setup
	inBox[0].construct(sf::Vector2f(20.0, 40.0), sf::Vector2f(960.0, 50.0), sf::Color(100, 100, 100, 255), sf::Color(240, 240, 240, 255), "Type Here...");

	button[0].construct(sf::Vector2f(160.0, 100.0), sf::Vector2i(20, 17), sf::Vector2f(2.0, 2.0), "1step.png", true, false);
	button[1].construct(sf::Vector2f(110.0, 100.0), sf::Vector2i(20, 17), sf::Vector2f(2.0, 2.0), "backstep.png", true, false);
	button[2].construct(sf::Vector2f(20.0, 100.0), sf::Vector2i(30, 17), sf::Vector2f(2.0, 2.0), "fullstep.png", true, false);
	button[3].construct(sf::Vector2f(940.0, 5.0), sf::Vector2i(19, 15), sf::Vector2f(2.0, 2.0), "consolebutton.png", false, true);

	txBox[0].construct(sf::Vector2f(20.0, 20.0), sf::Vector2f(0.0, 0.0), sf::Color(0, 0, 0, 0), sf::Color::Yellow, "Input:");
	txBox[1].construct(sf::Vector2f(20.0, 160.0), sf::Vector2f(0.0, 0.0), sf::Color(0, 0, 0, 0), sf::Color::Yellow, "Output:");
	txBox[2].construct(sf::Vector2f(20.0, 180.0), sf::Vector2f(960.0, 50.0), sf::Color(75, 75, 75, 255), sf::Color(0, 255, 255, 255), "");
	txBox[3].construct(sf::Vector2f(20.0, 280.0), sf::Vector2f(0.0, 0.0), sf::Color(0, 0, 0, 0), sf::Color::Yellow, "Operator Stack:");
	txBox[4].construct(sf::Vector2f(20.0, 380.0), sf::Vector2f(0.0, 0.0), sf::Color(0, 0, 0, 0), sf::Color::Yellow, "Operand Stack:");
	txBox[5].construct(sf::Vector2f(0.0, 765.0), sf::Vector2f(0.0, 0.0), sf::Color(0, 0, 0, 0), sf::Color(200, 200, 0, 100), "FLEE v1.0 by Andrei Filip\nGUI built using SFML 2.5");

	clBox[0].construct(sf::Vector2f(20.0, 300.0), sf::Vector2f(960.0, 60.0), sf::Color(0, 0, 0, 255));
	clBox[1].construct(sf::Vector2f(20.0, 400.0), sf::Vector2f(960.0, 60.0), sf::Color(0, 0, 0, 255));

	drawText("FLEE v1.0\n\n", 8, 0);
}

int main()
{
	ShowWindow(::GetConsoleWindow(), SW_HIDE);

	sf::RenderWindow window(sf::VideoMode(WINDOW_SIZE_X, WINDOW_SIZE_Y), "FLEE 1.0", sf::Style::Close);
	window.setFramerateLimit(30);
	sf::Image icon;
	icon.loadFromFile("graph.png");
	window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());

	sf::RectangleShape mainBackground(sf::Vector2f(WINDOW_SIZE_X - WINDOW_SIZE_Y, WINDOW_SIZE_Y));
	mainBackground.setFillColor(sf::Color(0, 50, 20, 255));

	sf::Mouse mouse;

	InputBox inBox[1]; int nrIB = 1, selectedIB = -1;
	Button button[4]; int nrBut = 4;
	TextBox txBox[6]; int nrTB = 6;
	ColorBox clBox[2]; int nrCB = 2;
	_setup(inBox, button, txBox, clBox);
	
	Tree tree; int archiveViewStep = 0;
	bool st_STEPVIEW = false, st_LOGVIEW = false;

	while (window.isOpen())
	{
		//cout << mouse.getPosition(window).x << ' ' << mouse.getPosition(window).y << '\n';
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::MouseButtonReleased)
			{
				sf::Vector2i mPos = mouse.getPosition(window);
				if (button[0].containsPoint(mPos))
				{
					button[0].onRelease();
					++archiveViewStep;
					if (archiveViewStep >= tree.archive.size / 2) //reached end
					{
						archiveViewStep = 0;
					}
				}
				else if (button[1].containsPoint(mPos))
				{
					button[1].onRelease();
					--archiveViewStep;
					if (archiveViewStep < 0) //reached end
					{
						archiveViewStep = tree.archive.size / 2 - 1;
					}
				}
				else if (button[2].containsPoint(mPos)) //construct button
				{
					button[2].onRelease();
					if (inBox[0].textStr.size())
					{
						drawText("For: ", 7, 0); drawText(inBox[0].textStr + "\n", 15, 0);
						//remove spaces
						string aux = inBox[0].textStr; int size = aux.size(); bool isComputable = true;
						for (int i = 0; i < size; ++i)
						{
							if (aux[i] == ' ')
								aux.erase(i, 1);
							else if (isComputable && !(aux[i] >= '0' && aux[i] <= '9') && !tree.isOperator(aux[i])) //find if computable
								isComputable = false;
						}
						int err = tree.construct(aux);
						if (!err)
						{
							drawText("Success!\n", 10, 0); drawText("Prefix notation: ", 7, 0); drawText(tree.getPrefixNotation(tree.root), 10, 0); drawText("\n", 8, 0);
							drawText("Value: ", 7, 0);
							if (isComputable) drawText(to_string(tree.getValue(tree.root)), 10, 0);
							else drawText("?", 4, 0);
							drawText("\n", 15, 0);

							tree.printTree();
							txBox[2].setText(tree.getPrefixNotation(tree.root));
							txBox[2].text.setFillColor(txBox[2].textCol);
							st_STEPVIEW = true;
							archiveViewStep = 0;

							//compute
						}
						else if (err == 1)
						{
							drawText("Error: ",7, 0); drawText("Operator stack is not empty\n", 4, 0);
							txBox[2].setText("Operator stack not empty");
							txBox[2].text.setFillColor(sf::Color(170, 0, 0, 255));
						}
						else if (err == 2)
						{
							drawText("Error: ", 7, 0); drawText("Operand stack is not empty\n", 4, 0);
							txBox[2].setText("Operand stack not empty");
							txBox[2].text.setFillColor(sf::Color(170, 0, 0, 255));
						}
						else if (err == 3)
						{
							drawText("Error: ", 7, 0); drawText("Unpaired parenthesis found\n", 4, 0);
							txBox[2].setText("Unpaired parenthesis");
							txBox[2].text.setFillColor(sf::Color(170, 0, 0, 255));
						}
						else if (err == 4)
						{
							drawText("Error: ", 7, 0); drawText("Missing operand\n", 4, 0);
							txBox[2].setText("Missing operand");
							txBox[2].text.setFillColor(sf::Color(170, 0, 0, 255));
						}
						else if (err == 5)
						{
							drawText("Error: ", 7, 0); drawText("Wrong syntax\n", 4, 0);
							txBox[2].setText("Wrong syntax");
							txBox[2].text.setFillColor(sf::Color(170, 0, 0, 255));
						}
						else
						{
							drawText("Error: ", 7, 0); drawText("Unknown error\n", 4, 0);
							txBox[2].setText("Unknown error");
							txBox[2].text.setFillColor(sf::Color(255, 50, 0, 255));
						}
						drawText("--------------------------------\n\n", 7, 0);
					}
				}
				else if (button[3].containsPoint(mPos))
				{
					button[3].onRelease();
					if (st_LOGVIEW)
						ShowWindow(::GetConsoleWindow(), SW_HIDE);
					else
					{
						ShowWindow(::GetConsoleWindow(), SW_SHOW);
						SetConsoleCursorInfo(hOutput, &cursorInfo);
					}
					st_LOGVIEW = !st_LOGVIEW;
				}
				else
				{
					for (int i = 0; i < nrBut; ++i) //realease stuck buttons
						if (!button[i].isSwitch)
							button[i].onRelease();
					tree.setHighlight(false);
				}

				int size = tree.archive.gStack.boxList.size();
				for (int i = 0; i < size; ++i)
					if (tree.archive.gStack.boxList[i].containsPoint(mPos) && tree.archive.gStack.boxList[i].textStr[0] == '@')
					{
						string aux = tree.archive.gStack.boxList[i].textStr; aux.erase(0, 1);
						tree.setHighlight(true, stoi(aux));
					}
			}
			if (event.type == sf::Event::MouseButtonPressed)
			{
				sf::Vector2i mPos = mouse.getPosition(window);
				bool clickedTB = false;
				for (int i = 0; i < nrIB; i++)
					if (inBox[i].containsPoint(mPos))
					{
						if (selectedIB != -1)
							inBox[selectedIB].isSelected = false;
						selectedIB = i;
						inBox[selectedIB].isSelected = true;
						clickedTB = true;
						inBox[i].blink = 5;
						break;
					}
				if (!clickedTB && selectedIB != -1)
				{
					inBox[selectedIB].isSelected = false;
					selectedIB = -1;
				}

				for (int i = 0; i < nrBut; ++i)
					if (button[i].containsPoint(mPos))
					{
						button[i].onPress();
						break;
					}
			}
			else if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == sf::Keyboard::Right)
				{
					if (selectedIB != -1)
						inBox[selectedIB].moveCursor(1);
					else
						tree.move(sf::Vector2f(-15.0, 0.0));
				}
				else if (event.key.code == sf::Keyboard::Left)
				{
					if (selectedIB != -1)
						inBox[selectedIB].moveCursor(-1);
					else
						tree.move(sf::Vector2f(15.0, 0.0));
				}
				else if (event.key.code == sf::Keyboard::Up)
				{
					tree.move(sf::Vector2f(0.0, 15.0));
				}
				else if (event.key.code == sf::Keyboard::Down)
				{
					tree.move(sf::Vector2f(0.0, -15.0));
				}
				else if (event.key.code == sf::Keyboard::C && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
				{
					sf::Vector2i mPos = mouse.getPosition(window);
					if (selectedIB != -1 && inBox[selectedIB].textStr.size())
						copyToClipboard(inBox[selectedIB].textStr);
					else if (txBox[2].textStr.size() && txBox[2].containsPoint(mPos))
						copyToClipboard(txBox[2].textStr);
				}
				else if (event.key.code == sf::Keyboard::V && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
				{
					if (selectedIB != -1)
					{
						inBox[selectedIB].textStr = getFromClipboard();
						inBox[selectedIB].moveCursor(inBox[selectedIB].textStr.size() - inBox[selectedIB].cursorPos);
					}
				}
			}
			else if (event.type == sf::Event::TextEntered)
			{
				if (selectedIB != -1)
				{
					if (inBox[selectedIB].modifyString(event.text.unicode)) //enter
						selectedIB = -1;
				}
			}
			else if (event.type == sf::Event::Closed)
				window.close();
		}
		window.clear();
		if (st_STEPVIEW)
			tree.draw(window);
		window.draw(mainBackground);
		for (int i = 0; i < nrIB; i++)
			inBox[i].draw(window);
		for (int i = 0; i < nrBut; i++)
			button[i].draw(window);
		for (int i = 0; i < nrTB; i++)
			txBox[i].draw(window);
		for (int i = 0; i < nrCB; i++)
			clBox[i].draw(window);

		if (st_STEPVIEW)
		{
			tree.archive.draw(sf::Vector2f(30.0, 310.0), sf::Vector2f(30.0, 410.0), archiveViewStep, window);
		}
		window.display();
	}

	return 0;
}
