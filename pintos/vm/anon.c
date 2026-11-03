/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "threads/vaddr.h"
#include "bitmap.h"

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static struct bitmap *swap_table;
static struct lock swap_lock;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

// 섹터 계산하는 매크로임 (김태규 피셜 좋다)
#define SECTORS_PER_PAGE (PGSIZE / DISK_SECTOR_SIZE)

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void
vm_anon_init (void) {

	/* vm_anon_init 에서 swap_disk 에 디스크 받기 */
	swap_disk = disk_get (1,1);

	/* bitmap 만들기 스왑슬롯 갯수 = 섹터 갯수 / ( 페이지 / 디스크의 섹터) */
	swap_table = bitmap_create (disk_size(swap_disk) / (PGSIZE / DISK_SECTOR_SIZE));

	/* swap_lock 초기화 */
	lock_init (&swap_lock);

	/* swap_table NULL 일시 커널패닉 */
	if (swap_table == NULL) {
		PANIC ("swap_table init fail");
	}

}

/* Initialize the file mapping */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
	
	/* uninit 때 값이 남아서 destroy가 엉뚱한 칸을 비움 */
	anon_page->slot_num = BITMAP_ERROR;
	
	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;

}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}
