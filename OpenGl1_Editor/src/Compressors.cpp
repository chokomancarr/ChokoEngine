#include "Compressors.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <bitset>

int IndexInDic (std::string& str, std::vector<std::string>& dic) {
    for (unsigned short x = dic.size(); x > 0; x--) {
        if (strcmp(dic[x-1].c_str(), str.c_str()) == 0)
            return x-1;
    }
    return -1;
}

void Compressors::Compress_LZW (unsigned char* input, unsigned int inSize, std::vector<unsigned char>& output) {
    output.clear();
	std::vector<std::string> dictionary = std::vector<std::string>();
	dictionary.push_back(""); //0=end
	for (unsigned short x = 0; x < 256; x++) {
		char xx[] = { (char)x, '\0' };
		dictionary.push_back(std::string(xx));
	}
	unsigned int outSize = 0;

    bool odd = false;
    std::string buffer = "";
	for (unsigned int a = 0; a < inSize; a++) {
		char xx[] = { (char)input[a], '\0' };
		std::string k = std::string(xx);
        int i = IndexInDic(buffer + k, dictionary);
        if (i > -1) {
            buffer += k;
			if (a == inSize - 1) {
				unsigned short o = (unsigned short)IndexInDic(buffer, dictionary);
				//cout << hex << o << endl;
				output.push_back((unsigned char)(o & 255));
				if (!odd) {
					output.push_back((unsigned char)((o & (15 << 8)) >> 4));
					outSize++;
				}
				else
					output[outSize - 1] |= ((unsigned char)(o >> 8));
				outSize++;
				odd = !odd;
			}
        }
        else {
			unsigned short o = (unsigned short)IndexInDic(buffer, dictionary);
			//cout << hex << o << endl;
			output.push_back((unsigned char)(o & 255));
			if (!odd) {
				output.push_back((unsigned char)((o & (15 << 8)) >> 4));
				outSize++;
			}
			else
				output[outSize - 1] |= ((unsigned char)(o >> 8));
			odd = !odd;
			outSize++;

			dictionary.push_back(buffer + k);
			buffer = k;
        }
    }

	output.push_back((unsigned char)(0));
	outSize++;
	if (!odd) {
		output.push_back((unsigned char)(0));
		outSize++;
	}

	//for (std::string sss : dictionary)
		//cout << sss << endl;
}

void Compressors::Decompress_LZW(std::ifstream& strm, std::vector<unsigned char>& output) {
	std::string buffer = "";
	std::vector<std::string> dictionary = std::vector<std::string>();
	dictionary.push_back(""); //0=end
	for (unsigned short x = 0; x < 256; x++) {
		char xx[] = { (char)x, '\0' };
		dictionary.push_back(std::string(xx));
	}
	unsigned short dicSize = 257;
	output.clear();
	unsigned short prevCode, currCode;
	bool isOdd = true;
	unsigned char byte0, byte1;
	std::string s = "";

	byte0 = strm.get();
	byte1 = strm.get();
	prevCode = byte0 | ((byte1 & 0xf0) >> 4);
	s = dictionary[prevCode];
	for (auto c : s) {
		output.push_back((unsigned char)c);
	}

	while (!strm.eof()) {
		if (!isOdd) {
			byte0 = strm.get();
			byte1 = strm.get();
			currCode = byte0 | (((unsigned short)(byte1 & 0xf0)) << 4);
		}
		else {
			byte0 = strm.get();
			currCode = byte0 | (((unsigned short)(byte1 & 0x0f)) << 8);
		}
		isOdd = !isOdd;
		//cout << hex << currCode << endl;
		if (currCode == 0) return;

		if (currCode == dicSize) {
			s = dictionary[prevCode];
			s = s + s.substr(0, 1);
			for (auto c : s) {
				output.push_back((unsigned char)c);
			}
			dictionary.push_back(s);
		}
		else {
			s = dictionary[currCode];
			for (auto c : s) {
				output.push_back((unsigned char)c);
			}
			dictionary.push_back(dictionary[prevCode] + s.substr(0, 1));
		}
		prevCode = currCode + 0;
		dicSize++;
	}
}

struct ch_node {
	ch_node(byte b) : parent(0), node_l(0), node_r(0), val(b), weight(0) {}
	ch_node(ch_node* a, ch_node* b) : parent(0), node_l(a), node_r(b), val(0), weight(a->weight + b->weight) {
		a->parent = b->parent = this;
	}

	ch_node* parent, *node_l, *node_r;
	byte val;
	uint weight;
};

void ch_bittree(std::vector<bool>& bits, ch_node* node) {
	if (node->node_l) {
		bits.push_back(0);
		ch_bittree(bits, node->node_l);
		ch_bittree(bits, node->node_r);
	}
	else {
		bits.push_back(1);
		auto b = std::bitset<8>(node->val);
		for (int i = 0; i < 8; i++) bits.push_back(b[i]);
	}
}
void ch_bitval(std::vector<bool>& bits, ch_node* node, std::vector<ch_node*> tree) {
	int x = 0;
	while (node->parent) {
		tree.push_back(node);
		node = node->parent;
		x++;
	}
	tree.push_back(node);
	while (x > 0) {
		bits.push_back(tree[x - 1] == tree[x]->node_r);
		x--;
	}
	int i = 0;
}

std::vector<byte> Compressors::Compress_Huffman(byte* input, uint inSize) {
	std::vector<ch_node*> nodes;
	nodes.reserve(256);
	for (uint i = 0; i < 256; i++) {
		nodes.push_back(new ch_node(i));
	}
	for (uint i = 0; i < inSize; i++) {
		nodes[input[i]]->weight++;
	}
	
	auto nodes2(nodes);
	for (int i = 255; i >= 0; i--) {
		if (nodes2[i]->weight == 0) {
			nodes2.erase(nodes2.begin() + i);
		}
	}
	byte x = nodes2.size()-1;
	while (x > 0) {
		std::sort(nodes2.begin(), nodes2.end(), [](ch_node* a, ch_node* b) {
			return (a->weight > b->weight);
		});
		ch_node* a = nodes2[x--];
		ch_node* b = nodes2[x];
		nodes2.pop_back();
		nodes2[x] = new ch_node(a, b);
	}

	std::vector<bool> bitmaps[256];
	byte bitmapSz[256];
	for (uint i = 0; i < 256; i++) {
		ch_bitval(bitmaps[i], nodes[i], std::vector<ch_node*>());
		bitmapSz[i] = bitmaps[i].size();
	}

	std::vector<bool> bits;
	bits.reserve(inSize*4);
	ch_bittree(bits, nodes2[0]); //recursive tree generation
	auto b = std::bitset<32>(inSize);
	for (int i = 0; i < 32; i++) bits.push_back(b[i]);
	for (uint i = 0; i < inSize; i++) { //characters
		//bits.insert(bits.end(), bitmaps[input[i]].begin(), bitmaps[input[i]].end());
	}
	
	uint sz = bits.size();
	/*
	byte pad = 8 - sz % 8;
	if (pad == 8) pad = 0;
	bits[0] = !!(pad & 4);
	bits[1] = !!(pad & 2);
	bits[2] = !!(pad & 1);
	*/
	std::vector<byte> res;
	res.reserve(sz / 8 + inSize / 2 + 1);
	byte bt = 0, off = 7;
	for (uint i = 0; i < sz; i++) {
		bt |= (((byte)bits[i]) << off--);
		if (off == 255) {
			res.push_back(bt);
			bt = 0;
			off = 7;
		}
	}
	for (uint i = 0; i < inSize; i++) { //characters
		for (byte j = 0; j < bitmapSz[input[i]]; j++) {
			bt |= (((byte)bitmaps[input[i]][j]) << off--);
			if (off == 255) {
				res.push_back(bt);
				bt = 0;
				off = 7;
			}
		}
	}
	if (off != 7) res.push_back(bt);
	for (byte i = 0; i < 256; i++) delete(nodes[i]);
	return res;
}

void dh_bittree(std::vector<bool>& bits, uint& off, ch_node* me) {
	if (bits[off++]) { //this is leaf
		byte bt = 0;
		for (byte b = 0; b < 8; b++)
			bt |= ((byte)bits[off++] << b);
		me->val = bt;
	}
	else { //this is node
		me->node_l = new ch_node(0);
		me->node_r = new ch_node(0);
		dh_bittree(bits, off, me->node_l);
		dh_bittree(bits, off, me->node_r);
	}
}
byte dh_bitbyte(std::vector<bool>& bits, uint& off, ch_node* tree) {
	while (tree->node_l) {
		tree = (bits[off++] ? tree->node_r : tree->node_l);
	}
	return tree->val;
}
std::vector<byte> Compressors::Decompress_Huffman(byte* input, uint inSize) {
	std::vector<bool> bits;
	bits.reserve(inSize * 8);
	for (uint a = 0; a < inSize; a++)
		for (char b = 7; b >= 0; b--)
			bits.push_back(!!(input[a] & (1 << b)));
	uint off = 0;
	ch_node* node = new ch_node(0);
	dh_bittree(bits, off, node);
	
	uint num = 0;
	for (byte a = 0; a < 32; a++) {
		num |= bits[off++] << a;
	}
	std::vector<byte> res;
	res.reserve(num);
	while (num > 0) {
		res.push_back(dh_bitbyte(bits, off, node));
		num--;
	}
	return res;
}
