// stdafx.h: ���������� ���� ��� ����������� ��������� ���������� ������
// ��� ���������� ������ ��� ����������� �������, ������� ����� ������������, ��
// �� ����� ����������
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include <iostream>
#include <vector>
#include <queue>
using namespace std;

// TODO: ���������� ����� ������ �� �������������� ���������, ����������� ��� ���������
#include "mpi.h"

#define __foreach(it, i, c) for(it i = c.begin(), _end_##i = c.end(); i != _end_##i; ++i)