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
	
	ASSERT (anon_page->slot_num != BITMAP_ERROR);

	/* slot_num 슬롯의 섹터 내용 읽고 메모리 적재 */
	for (int i = 0; i < (PGSIZE / DISK_SECTOR_SIZE); i++) {
		disk_read (swap_disk, anon_page->slot_num * (PGSIZE / DISK_SECTOR_SIZE) + i, page->frame->kva + i * DISK_SECTOR_SIZE);
	}
	
	/* swap_table의 slot_num 슬롯 비우기 */
	lock_acquire (&swap_lock);
		bitmap_set (swap_table, anon_page->slot_num, false);
	lock_release (&swap_lock);

	/* 페이지에 스왑에 없음 표시 */
	anon_page->slot_num = BITMAP_ERROR;

	return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;

	/* 스왑 락을 잡고 돌려야 함
	섹터 8개를 기준으로 슬롯을 돌면서 빈걸 찾아야 하는데
	플립 내장함수 사용
	못찾으면 false 반환
	찾았으면 스왑디스크에 물리 페이지 값을 그대로 넣기
	페이지랑 물리페이지 매핑 해제해줘야 함
	*/
	size_t slot;
	lock_acquire(&swap_lock);
	slot = bitmap_scan_and_flip(swap_table, 0, 1, false);
	lock_release(&swap_lock);

	// 실패 예외 처리
	if(slot == BITMAP_ERROR){
		return false;
	}

	pml4_clear_page(page->cur_t->pml4, page->va);

	for (int i = 0; i < SECTORS_PER_PAGE; i++){
		// 섹터 번호 : slot * SECTORS_PER_PAGE + i
		// 버퍼 : page->frame->kva + i * DISK_SECTOR_SIZE
		// 슬롯의 섹터 번호에 버퍼의 메모리를 복사하는 것이다.
		disk_write(swap_disk, slot * SECTORS_PER_PAGE + i, page->frame->kva + i * DISK_SECTOR_SIZE);
	}

	anon_page->slot_num = slot;
	// 성공하면 연결을 해제해줘야 한다.
	page->frame->page = NULL;
	page->frame = NULL;

	return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}
