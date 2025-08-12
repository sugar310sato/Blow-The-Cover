#pragma once
#include <new>
#include <cstddef>

// class Tを最大MAXSIZE個確保可能なPoolAllocatorを実装してください
template<class T, size_t MAXSIZE> class PoolAllocator
{
public:
	// コンストラクタ
	PoolAllocator() {
		// TODO: 必要に応じて実装してください

		freeList = reinterpret_cast<FreeNode*>(pool); // フリーリストを初期化　pool(char配列)の先頭をフリーリストの先頭に設定

		// 全てのノードをフリーリストに連結
		for (size_t i = 0; i < MAXSIZE - 1; ++i) {
			reinterpret_cast<FreeNode*>(pool + i * sizeof(FreeNode))->next = //: i番目のFreeNodeの位置を計算しnextポインタに
				reinterpret_cast<FreeNode*>(pool + (i + 1) * sizeof(FreeNode)); //i+1番目のFreeNodeのアドレスをせっててい
		}

		// 最後のノードのnextはnullptrに設定
		reinterpret_cast<FreeNode*>(pool + (MAXSIZE - 1) * sizeof(FreeNode))->next = nullptr;
	}

	// デストラクタ
	~PoolAllocator() {
		// TODO: 必要に応じて実装してください
	}

	// 確保できない場合はnullptrを返す事。
	T* Alloc() {
		// TODO: 実装してください
		if (freeList == nullptr) {
			return nullptr; // フリーリストが空の場合はnullptrを返す
		}

		// フリーリストの先頭から1つ取得
		FreeNode* node = freeList;
		freeList = freeList->next; //先頭の更新
		// placement new でT型のコンストラクタを呼び出し、オブジェクトを生成
		T* obj = reinterpret_cast<T*>(node);
		return new (obj) T();
	}

	// Free(nullptr)で誤動作しないようにする事。
	void Free(T* addr) {
		// TODO: 実装してください
		if (addr == nullptr) {
			return; // nullptrは何もしない
		}

		if (addr < reinterpret_cast<T*>(pool) ||
			addr >= reinterpret_cast<T*>(pool + sizeof(FreeNode) * MAXSIZE)) {
			return; // 無効なアドレスの場合は何もしない
		}
		// Tのデストラクタを呼び出す
		addr->~T();

		FreeNode* node = reinterpret_cast<FreeNode*>(addr);
		node->next = freeList; // フリーリストの先頭に追加
		freeList = node; // フリーリストの更新
	}

private:
	// TODO: 実装してください
	union FreeNode { // フリーリストのノード構造体
		FreeNode* next; // 次のノードへのポインタ
		alignas(T) char data[sizeof(T)];
	};

	alignas(T) char pool[sizeof(FreeNode) * MAXSIZE];

	FreeNode* freeList;
};
