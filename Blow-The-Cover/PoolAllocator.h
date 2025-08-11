#pragma once
#include <new>
#include <cstddef>

// class T���ő�MAXSIZE�m�ۉ\��PoolAllocator���������Ă�������
template<class T, size_t MAXSIZE> class PoolAllocator
{
public:
	// �R���X�g���N�^
	PoolAllocator() {
		// TODO: �K�v�ɉ����Ď������Ă�������

		freeList = reinterpret_cast<FreeNode*>(pool); // �t���[���X�g���������@pool(char�z��)�̐擪���t���[���X�g�̐擪�ɐݒ�

		// �S�Ẵm�[�h���t���[���X�g�ɘA��
		for (size_t i = 0; i < MAXSIZE - 1; ++i) {
			reinterpret_cast<FreeNode*>(pool + i * sizeof(FreeNode))->next = //: i�Ԗڂ�FreeNode�̈ʒu���v�Z��next�|�C���^��
				reinterpret_cast<FreeNode*>(pool + (i + 1) * sizeof(FreeNode)); //i+1�Ԗڂ�FreeNode�̃A�h���X�������ĂĂ�
		}

		// �Ō�̃m�[�h��next��nullptr�ɐݒ�
		reinterpret_cast<FreeNode*>(pool + (MAXSIZE - 1) * sizeof(FreeNode))->next = nullptr;
	}

	// �f�X�g���N�^
	~PoolAllocator() {
		// TODO: �K�v�ɉ����Ď������Ă�������
	}

	// �m�ۂł��Ȃ��ꍇ��nullptr��Ԃ����B
	T* Alloc() {
		// TODO: �������Ă�������
		if (freeList == nullptr) {
			return nullptr; // �t���[���X�g����̏ꍇ��nullptr��Ԃ�
		}

		// �t���[���X�g�̐擪����1�擾
		FreeNode* node = freeList;
		freeList = freeList->next; //�擪�̍X�V
		// placement new ��T�^�̃R���X�g���N�^���Ăяo���A�I�u�W�F�N�g�𐶐�
		T* obj = reinterpret_cast<T*>(node);
		return new (obj) T();
	}

	// Free(nullptr)�Ō듮�삵�Ȃ��悤�ɂ��鎖�B
	void Free(T* addr) {
		// TODO: �������Ă�������
		if (addr == nullptr) {
			return; // nullptr�͉������Ȃ�
		}

		if (addr < reinterpret_cast<T*>(pool) ||
			addr >= reinterpret_cast<T*>(pool + sizeof(FreeNode) * MAXSIZE)) {
			return; // �����ȃA�h���X�̏ꍇ�͉������Ȃ�
		}
		// T�̃f�X�g���N�^���Ăяo��
		addr->~T();

		FreeNode* node = reinterpret_cast<FreeNode*>(addr);
		node->next = freeList; // �t���[���X�g�̐擪�ɒǉ�
		freeList = node; // �t���[���X�g�̍X�V
	}

private:
	// TODO: �������Ă�������
	union FreeNode { // �t���[���X�g�̃m�[�h�\����
		FreeNode* next; // ���̃m�[�h�ւ̃|�C���^
		alignas(T) char data[sizeof(T)];
	};

	alignas(T) char pool[sizeof(FreeNode) * MAXSIZE];

	FreeNode* freeList;
};
