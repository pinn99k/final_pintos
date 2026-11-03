/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include "userprog/syscall.h"

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
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
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
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
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
