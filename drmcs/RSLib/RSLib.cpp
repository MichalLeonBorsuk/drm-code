/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2006
 *
 * Author(s):  Julian Cable, Ollie Haffenden, Andrew Murphy
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/
// RSLib.cpp : Defines the entry point for the console application.
//


//#include "GaloisField.h"
//#include "FiniteFieldElement.h"
//#include "FiniteFieldPolynomial.h"
//#include "RSAlgebra.h"
#include "RSCodePFT.h"
#include "rs.h"


#include <iostream>
using namespace std;
#ifdef _MSC_VER
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
/*	//const unsigned int FieldSizeLog2 = 8;
	const unsigned int FieldSizeLog2 = 4;
	const unsigned int FieldSize = 1 << FieldSizeLog2;
	//GaloisField field(FieldSizeLog2, 0x11D);
	GaloisField field(FieldSizeLog2, 0x13);
	FiniteFieldElement x = field.GetElement(10);
	FiniteFieldElement y = field.GetElement(13);
	FiniteFieldElement z = field.GetElement(11);
	cout << "x=" << x << " y= " << y << " z = " << z<< endl;
	cout << "x + y is " << x+y << endl;
	cout << "x * y is " << x*y << endl;
	cout << "z / x is " << z/x << endl;

	cout << "Multiplication table" << endl;
	for (int xval=0; xval<16; xval++)
	{
		for (int yval=0; yval<16; yval++)
		{
			x = field.GetElement(xval);
			y = field.GetElement(yval);
			cout << x*y << "\t";
		}
		cout << endl;
	}

//	FiniteFieldPolynomial g(&field, field.GetElement(1), 0);
//	cout << g;
//	for (int i = 1; i<49; i++)
//		g.AddRoot(field.AlphaToPower(i));
//	cout << g;

//	cout << "g(1) = " << g(field.GetElement(1)) << endl;
//	cout << "g(2) = " << g(field.GetElement(2)) << endl;
//	cout << "g(3) = " << g(field.GetElement(3)) << endl;
//	cout << "g(17) = " << g(field.GetElement(17)) << endl;
//	cout << "g(8) = " << g(field.GetElement(8)) << endl;
//	cout << "g(132) = " << g(field.GetElement(132)) << endl;

	FiniteFieldPolynomial p(&field);
	for (int i=0; i<11; i++)
		p.SetCoefficient(i, field.GetElement(11-i));


	//RSAlgebra rsalg(25,20, &field);
	RSAlgebra rsalg(15,11, &field,0);
	rsalg.Encode(p);

	cout << "coded polynomial = " <<endl << p<<endl;

	cout << "No errors: decoding:" <<endl;
	rsalg.Decode(p);

	p.SetCoefficient(9, field.GetElement(11));
	p.SetCoefficient(2, field.GetElement(1));
	cout << "error at 2 and 9: decoding:"<<endl;
	rsalg.Decode(p);
	cout << "Corrected data:"<<endl<<p<<endl;
*/

	// Julian: you should just need to #include "RSCodePFT.h", then instantiate a CRSCodePFT:
	CRSCodePFT code;

	unsigned char data[207];
	unsigned char parity1[48];
	unsigned char parity2[48];

	// Generate some test input data
	for (int i = 0; i<207; i++)
		data[i] = i;

	// encode with both coders
	encode_rs(data, parity1);	

	code.Encode(data, parity2);

	// print results - should be the same
	for (int i = 0; i<48; i++)
		cout<<i<<":\t"<<(int)parity1[i]<<"\t"<<(int)parity2[i]<<endl;
#ifdef WIN32
	system("PAUSE");
#endif
}

