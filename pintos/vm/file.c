/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include "userprog/syscall.h"
#include "lib/round.h"
#include "threads/malloc.h"
#include <string.h>

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	struct vm_entry *src = page->uninit.aux;

	page->operations = &file_ops;
	struct file_page *file_page = &page->file;
	file_page->aux = malloc(sizeof(struct vm_entry));

	if(file_page->aux == NULL){
		return false;
	}
	memcpy(file_page->aux, src, sizeof(struct vm_entry));

	return true;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;

	// 더티비트인지 확인해서 저장하기
	bool dirty = pml4_is_dirty(page->cur_t->pml4, page->va);

	// pml4 매핑 해제
	pml4_clear_page(page->cur_t->pml4, page->va);

	// 더티비트 확인해서 옮기기
	if(dirty){
		// 더티비트면 KVA 값을 읽어서 파일에 쓰기
		lock_acquire(&file_lock);
		if (file_page->aux->file != NULL) {
			file_write_at (file_page->aux->file, page->frame->kva, file_page->aux->read_bytes, file_page->aux->ofs);
		}
		lock_release(&file_lock);
	}
	
	// 연결 끊기
	page->frame->page = NULL;
	page->frame = NULL;
	
	// 리턴 트루
	return true;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page = &page->file;
	
	if (page->frame != NULL) {
		
		/* 프레임이 있을 때, pml4_is_dirty 면 -> file의 ofs에 read_bytes 만큼 쓰기 */ 
		if (pml4_is_dirty(thread_current()->pml4, page->va)) {
			lock_acquire(&file_lock);
			if (file_page->aux->file != NULL) {
				file_write_at (file_page->aux->file, page->frame->kva, file_page->aux->read_bytes, file_page->aux->ofs);
			}
			lock_release(&file_lock);
		}
			
		/* pml4_clear_page로 pml4 에서 등록해제 */
		pml4_clear_page(thread_current()->pml4, page->va);	
			
		/* 프레임 해제 */
		palloc_free_page(page->frame->kva);
		
	}
	
	/* file_close, file 객체 해제 */
	lock_acquire(&file_lock);
	if (file_page->aux->file != NULL) {
		file_close(file_page->aux->file);
	}	
	lock_release(&file_lock);      

	free(file_page->aux);
}

/* mmap 로딩 함수 */
static bool
mmap_load_segment (struct page *page, void *aux) {
	/* TODO: Load the segment from the file */
	/* TODO: This called when the first page fault occurs on address VA. */
	/* TODO: VA is available when calling this function. */
	struct vm_entry *_aux = aux;

	/* 파일 커서를 오프셋 위치로 옮겨준다.  */
	bool is_holding = lock_held_by_current_thread(&file_lock);
	if(!is_holding){
		lock_acquire(&file_lock);

		file_seek(_aux->file, _aux->ofs);

		/* 파일을 버퍼에 read_bytes 만큼 읽어들임. */
		if (_aux->read_bytes != 0 && file_read (_aux->file, page->frame->kva, _aux->read_bytes) != (int) _aux->read_bytes) {
			free (_aux);
			lock_release(&file_lock);
			return false;
		}

		lock_release(&file_lock);
	}
	else{
		file_seek(_aux->file, _aux->ofs);

		if (_aux->read_bytes != 0 && file_read (_aux->file, page->frame->kva, _aux->read_bytes) != (int) _aux->read_bytes) {
			free (_aux);
			return false;
		}
	}

	/* 물리메모리의 페이지에 read_bytes 만큼 채우고 나머지 0으로 채우기  */
	memset (page->frame->kva + _aux->read_bytes, 0, _aux->zero_bytes);

	/* vm_entry를 할당해제 시켜준다. (메모리누수 방지) */
	
	free (_aux);
	return true;
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
	struct supplemental_page_table *spt = &thread_current ()->spt;
	off_t file_len;
	bool is_holding = lock_held_by_current_thread(&file_lock);
	if(!is_holding){
		lock_acquire(&file_lock);
		file_len = file_length(file);
		lock_release(&file_lock);
	}
	else{
		file_len = file_length(file);
	}
	// 예외 처리
	if(addr == NULL || is_kernel_vaddr(addr) || length == 0){
		return NULL;
	}
	if(is_kernel_vaddr(addr + length -1) || addr > (addr + length -1)){
		return NULL;
	}

	// 오프셋이 정렬되어있는지 확인하기;;
	if((offset % PGSIZE) != 0){
		return NULL;
	}

	// 오프셋 검사해야함
	if(offset < 0 || offset > file_len){
		return NULL;
	}
	
	// 페이지가 정렬되어있는지 확인해야 함
	// 4KB 로 나눠서 페이지를 만들기 때문
	if(pg_ofs(addr) != 0){
		return NULL;
	}
	
	// spt에 등록된게 있으면 안됨
	for(uint8_t *va = pg_round_down(addr); va <= (addr + length -1); va += PGSIZE){
		struct page *find_page = spt_find_page(spt, va);

		if(find_page != NULL){
			return NULL;
		}
	}

	size_t file_size = file_len - offset;
	size_t read_bytes = length < file_size ? length : file_size;
	size_t zero_bytes = ROUND_UP(length, PGSIZE) - read_bytes;

	// 몇개의 페이지가 필요한지/ 
	int count = (length + PGSIZE - 1) / PGSIZE;
	struct vm_entry *vm_entry =  NULL;
	void * start = addr;
	for (int i = 0; i < count; i++){
		/* Do calculate how to fill this page.
		 * We will read PAGE_READ_BYTES bytes from FILE
		 * and zero the final PAGE_ZERO_BYTES bytes. */
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		is_holding = lock_held_by_current_thread(&file_lock);
		vm_entry = malloc (sizeof (struct vm_entry));
		// 새 파일을 넣어야 나중에 해제되지 않음
		if(!is_holding){
			lock_acquire(&file_lock);
			vm_entry->file = file_reopen(file);
			lock_release(&file_lock);
		}
		else{
			vm_entry->file = file_reopen(file);
		}
		vm_entry->ofs = offset;
		vm_entry->read_bytes = page_read_bytes;
		vm_entry->zero_bytes = page_zero_bytes;


		/* TODO: Set up aux to pass information to the lazy_load_segment. */
		void *aux = vm_entry;

		/* 페이지 초기화 및 spt에 등록 */
		if (!vm_alloc_page_with_initializer (VM_FILE, addr,
					writable, mmap_load_segment, aux))
			return NULL;

		/* Advance. */
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		addr += PGSIZE;
		offset += page_read_bytes;
	}
	struct page *page = spt_find_page(spt, pg_round_down(start));
	page->page_cnt = count;

	return start;
}

/* Do the munmap */
void
do_munmap (void *addr) {
	
	/* addr로 spt안에 페이지 찾기 */
	struct supplemental_page_table *spt = &thread_current() -> spt;
	struct page *find_page = spt_find_page (spt, addr);
	
	/* 찾은 페이지가 file 인지 확인 */
	if (find_page == NULL || page_get_type(find_page) != VM_FILE) {
		return;
	}
	
	/* 첫번째 페이지 삭제시 page_cnt 가 삭제되므로 지역변수에 저장 */
	int page_cnt = find_page->page_cnt;
	
	/* 매핑된 주소의 첫페이지 부터 spt에서 찾고 삭제  */
	for (int i = 0; i < page_cnt; i++) {
		find_page = spt_find_page (spt, addr + i * PGSIZE);

		if (find_page != NULL) {
			spt_remove_page (spt,find_page);
		}
	}

}
