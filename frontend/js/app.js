const shopItems = [];
const inventory = [];
let coins = 0;
let accountReady = false;

const inventoryPanel = document.querySelector('#inventory-panel');
const inventoryGrid = document.querySelector('#inventory-grid');
const inventoryCount = document.querySelector('#inventory-count');
const openInventoryButton = document.querySelector('#open-inventory');
const closeInventoryButton = document.querySelector('#close-inventory');
const inventoryTabs = document.querySelectorAll('#inventory-panel [data-filter]');
const inventoryDetail = document.querySelector('#inventory-detail');
let activeFilter = 'all';
let selectedBagId = null;
let bagMessage = '';
const shopPanel = document.querySelector('#shop-panel');
const shopGrid = document.querySelector('#shop-grid');
const shopMessage = document.querySelector('#shop-message');
const shopCoinBalance = document.querySelector('#shop-coin-balance');
const coinBalance = document.querySelector('#coin-balance');
const openShopButton = document.querySelector('#open-shop');
const closeShopButton = document.querySelector('#close-shop');
const shopTabs = document.querySelectorAll('#shop-panel [data-shop-filter]');
let activeShopFilter = 'food';
let purchaseDialog = null;

const decoratePanel = document.querySelector('#decorate-panel');
const decorateGrid = document.querySelector('#decorate-grid');
const decorateHint = document.querySelector('#decorate-hint');
const openDecorateButton = document.querySelector('#open-decorate');
const closeDecorateButton = document.querySelector('#close-decorate');
const decorateTabs = document.querySelectorAll('#decorate-panel [data-decor-slot]');
let activeDecorSlot = 'background';
const equippedDecor = { background: null, floor: null };
const lobbyScene = document.querySelector('#lobby-scene');
const lobbyFloor = document.querySelector('#lobby-floor');
const petGame = document.querySelector('.pet-game');

function updateCoinDisplays() {
	coinBalance.textContent = coins;
	shopCoinBalance.textContent = coins;
}

// Ảnh món đồ; nền/sàn không có ảnh nên vẽ bằng gradient theo data-scene / data-floor.
function buildItemArt(item) {
	const art = document.createElement('div');
	art.className = 'win-art';
	if (item.filter === 'background' || item.filter === 'floor') {
		art.dataset[item.filter === 'background' ? 'scene' : 'floor'] = String(item.id);
		return art;
	}
	const image = document.createElement('img');
	image.src = item.image;
	image.alt = item.name;
	image.draggable = false;
	image.addEventListener('error', () => {
		const fallback = document.createElement('span');
		fallback.className = 'art-fallback';
		fallback.setAttribute('aria-hidden', 'true');
		fallback.textContent = item.icon ?? '?';
		image.replaceWith(fallback);
	}, { once: true });
	art.append(image);
	return art;
}

function renderInventory() {
	const visibleItems = inventory.filter((item) => {
		return activeFilter === 'all' || item.category === activeFilter;
	});
	const totalQuantity = visibleItems.reduce((total, item) => total + item.quantity, 0);

	inventoryCount.textContent = `${totalQuantity} items`;
	inventoryGrid.replaceChildren();
	if (!visibleItems.some((item) => item.id === selectedBagId)) {
		selectedBagId = visibleItems[0]?.id ?? null;
	}

	if (visibleItems.length === 0) {
		const emptyState = document.createElement('p');
		emptyState.className = 'win-empty';
		emptyState.innerHTML = '<strong>Nothing here yet</strong>Visit the shop to find something for UIA.';
		inventoryGrid.append(emptyState);
		renderBagDetail();
		return;
	}

	visibleItems.forEach((item) => {
		const selected = item.id === selectedBagId;
		const cell = document.createElement('button');
		cell.type = 'button';
		cell.className = `bag-cell${selected ? ' is-selected' : ''}`;
		cell.setAttribute('aria-pressed', String(selected));

		const name = document.createElement('span');
		name.className = 'bag-name';
		name.textContent = item.name;
		cell.append(buildItemArt(item), name);

		if (item.category === 'food') {
			const quantity = document.createElement('span');
			quantity.className = 'bag-qty';
			quantity.textContent = `x${item.quantity}`;
			cell.append(quantity);
		}

		cell.addEventListener('click', () => {
			selectedBagId = item.id;
			bagMessage = '';
			renderInventory();
			inventoryGrid.querySelector('.is-selected')?.focus();
		});
		inventoryGrid.append(cell);
	});
	renderBagDetail();
}

function renderBagDetail() {
	inventoryDetail.replaceChildren();
	const item = inventory.find((entry) => entry.id === selectedBagId);
	if (!item) {
		const hint = document.createElement('p');
		hint.className = 'win-message';
		hint.textContent = 'Select an item to see its details.';
		inventoryDetail.append(hint);
		return;
	}

	const info = document.createElement('div');
	info.className = 'win-detail-text';
	const name = document.createElement('h2');
	name.textContent = item.name;
	const description = document.createElement('p');
	description.textContent = item.description;
	const message = document.createElement('p');
	message.className = 'win-message';
	message.setAttribute('role', 'status');
	message.textContent = bagMessage;
	info.append(name, description, message);

	const action = document.createElement('button');
	action.type = 'button';
	action.className = 'win-btn';
	if (item.category === 'food') {
		action.textContent = 'Use';
		action.addEventListener('click', () => useBagFood(item));
	} else if (item.filter === 'headwear') {
		action.textContent = 'Wear';
		action.disabled = true;
	} else {
		const worn = equippedDecor[item.filter] === item.id;
		action.textContent = worn ? 'Remove' : 'Equip';
		action.addEventListener('click', () => equipBagDecoration(item, worn));
	}
	inventoryDetail.append(buildItemArt(item), info, action);
}

async function useBagFood(item) {
	try {
		applyAccountState(await api.feedPet(item.id));
		bagMessage = `${item.name} eaten!`;
	} catch (failure) {
		bagMessage = failure.message;
	}
	renderBagDetail();
}

async function equipBagDecoration(item, worn) {
	try {
		applyAccountState(await api.equipDecoration(item.filter, worn ? null : item.id));
		bagMessage = worn ? `${item.name} removed.` : `${item.name} equipped.`;
	} catch (failure) {
		bagMessage = failure.message;
	}
	renderBagDetail();
}

function renderShop() {
	const visibleItems = shopItems.filter((item) => item.filter === activeShopFilter);
	shopGrid.replaceChildren();

	visibleItems.forEach((item) => {
		const owned = item.category === 'decoration' && inventory.some((ownedItem) => ownedItem.id === item.id);
		const row = document.createElement('article');
		row.className = `shop-row${owned ? ' is-owned' : ''}`;

		const text = document.createElement('div');
		text.className = 'shop-row-text';
		const name = document.createElement('h3');
		name.textContent = item.name;
		const description = document.createElement('p');
		description.textContent = item.description;
		text.append(name, description);

		const price = document.createElement('span');
		price.className = 'shop-row-price';
		const coinIcon = document.createElement('span');
		coinIcon.className = 'px-icon';
		coinIcon.dataset.px = 'coin';
		price.append(coinIcon, String(item.price));

		const buyButton = document.createElement('button');
		buyButton.type = 'button';
		buyButton.className = 'win-btn';
		buyButton.dataset.buyId = String(item.id);
		buyButton.disabled = owned;
		buyButton.textContent = owned ? 'Owned' : 'Buy';

		row.append(buildItemArt(item), text, price, buyButton);
		shopGrid.append(row);
	});
	applyPixelIcons(shopGrid);
}

function closePurchaseDialog() {
	if (!purchaseDialog) {
		return;
	}
	purchaseDialog.remove();
	purchaseDialog = null;
	closeShopButton.focus();
}

function openPurchaseDialog(item) {
	closePurchaseDialog();
	const backdrop = document.createElement('div');
	backdrop.className = 'purchase-backdrop';
	backdrop.addEventListener('click', (event) => {
		if (event.target === backdrop) {
			closePurchaseDialog();
		}
	});

	const dialog = document.createElement('section');
	dialog.className = 'purchase-dialog';
	dialog.setAttribute('role', 'dialog');
	dialog.setAttribute('aria-modal', 'true');
	dialog.setAttribute('aria-labelledby', 'purchase-title');

	const header = document.createElement('header');
	header.className = 'purchase-dialog-header';
	const title = document.createElement('h2');
	title.id = 'purchase-title';
	const isFood = item.category === 'food';
	title.textContent = isFood ? 'Choose quantity' : 'Confirm purchase';
	const closeButton = document.createElement('button');
	closeButton.type = 'button';
	closeButton.className = 'purchase-close';
	closeButton.setAttribute('aria-label', 'Close quantity window');
	closeButton.textContent = '×';
	closeButton.addEventListener('click', closePurchaseDialog);
	header.append(title, closeButton);

	const itemName = document.createElement('p');
	itemName.className = 'purchase-item-name';
	itemName.textContent = item.name;
	const unitPrice = document.createElement('p');
	unitPrice.className = 'purchase-unit-price';
	unitPrice.textContent = isFood ? `${item.price} coins each` : `One item · ${item.price} coins`;

	let quantityInput = null;
	let quantityLabel = null;
	if (isFood) {
		quantityLabel = document.createElement('label');
		quantityLabel.className = 'purchase-quantity-label';
		quantityLabel.htmlFor = 'purchase-quantity';
		quantityLabel.textContent = 'Quantity';
		quantityInput = document.createElement('input');
		quantityInput.className = 'purchase-quantity-input';
		quantityInput.id = 'purchase-quantity';
		quantityInput.type = 'number';
		quantityInput.min = '1';
		quantityInput.max = String(Math.max(1, Math.floor(coins / item.price)));
		quantityInput.value = '1';
		quantityInput.inputMode = 'numeric';
		quantityInput.autocomplete = 'off';
	}

	const total = document.createElement('p');
	total.className = 'purchase-total';
	total.textContent = `Total: ${item.price} coins`;
	const error = document.createElement('p');
	error.className = 'purchase-error';
	error.setAttribute('role', 'status');
	if (coins < item.price) {
		error.textContent = 'Not enough coins for this item.';
	}

	const actions = document.createElement('div');
	actions.className = 'purchase-actions';
	const cancelButton = document.createElement('button');
	cancelButton.type = 'button';
	cancelButton.className = 'purchase-cancel';
	cancelButton.textContent = 'Cancel';
	cancelButton.addEventListener('click', closePurchaseDialog);
	const confirmButton = document.createElement('button');
	confirmButton.type = 'button';
	confirmButton.className = 'purchase-confirm';
	confirmButton.textContent = 'Confirm purchase';
	confirmButton.addEventListener('click', async () => {
		const quantity = quantityInput ? Number.parseInt(quantityInput.value, 10) : 1;
		if (quantityInput && (!Number.isInteger(quantity) || quantity < 1 || quantity > Number(quantityInput.max))) {
			error.textContent = `Choose a quantity from 1 to ${quantityInput.max}.`;
			return;
		}

		const totalPrice = item.price * quantity;
		if (coins < totalPrice) {
			error.textContent = 'Not enough coins for this quantity.';
			return;
		}

		confirmButton.disabled = true;
		cancelButton.disabled = true;
		error.textContent = '';
		try {
			const result = await api.buyItem(item.id, quantity);
			applyAccountState(result);
			shopMessage.textContent = `${quantity} × ${item.name} added to your inventory.`;
			closePurchaseDialog();
		} catch (failure) {
			error.textContent = failure.message;
		} finally {
			confirmButton.disabled = false;
			cancelButton.disabled = false;
		}
	});
	actions.append(cancelButton, confirmButton);

	quantityInput?.addEventListener('input', () => {
		const maxQuantity = Number(quantityInput.max);
		let quantity = Number.parseInt(quantityInput.value, 10);
		if (Number.isInteger(quantity) && quantity > maxQuantity) {
			quantity = maxQuantity;
			quantityInput.value = String(quantity);
		}
		if (!Number.isInteger(quantity) || quantity < 1) {
			quantity = 1;
		}
		total.textContent = `Total: ${item.price * quantity} coins`;
		error.textContent = '';
	});

	dialog.append(header, itemName, unitPrice);
	if (isFood) {
		dialog.append(quantityLabel, quantityInput);
	}
	dialog.append(total, error, actions);
	backdrop.append(dialog);
	petGame.append(backdrop);
	purchaseDialog = backdrop;
	(quantityInput ?? confirmButton).focus();
}

function openInventory() {
	if (!accountReady) return;
	inventoryPanel.hidden = false;
	document.body.classList.add('inventory-open');
	closeInventoryButton.focus();
}

function closeInventory() {
	inventoryPanel.hidden = true;
	document.body.classList.remove('inventory-open');
	openInventoryButton.focus();
}

function openShop() {
	if (!accountReady) return;
	shopPanel.hidden = false;
	document.body.classList.add('inventory-open');
	shopMessage.textContent = '';
	renderShop();
	updateCoinDisplays();
	closeShopButton.focus();
}

function closeShop() {
	closePurchaseDialog();
	shopPanel.hidden = true;
	document.body.classList.remove('inventory-open');
	openShopButton.focus();
}

openInventoryButton.addEventListener('click', openInventory);
closeInventoryButton.addEventListener('click', closeInventory);
openShopButton.addEventListener('click', openShop);
closeShopButton.addEventListener('click', closeShop);

inventoryPanel.addEventListener('click', (event) => {
	if (event.target === inventoryPanel) {
		closeInventory();
	}
});

shopPanel.addEventListener('click', (event) => {
	if (event.target === shopPanel) {
		closeShop();
	}
});

document.addEventListener('keydown', (event) => {
	if (event.key === 'Escape' && purchaseDialog) {
		closePurchaseDialog();
		return;
	}
	if (event.key === 'Escape' && !inventoryPanel.hidden) {
		closeInventory();
	}
	if (event.key === 'Escape' && !shopPanel.hidden) {
		closeShop();
	}
	if (event.key === 'Escape' && !decoratePanel.hidden) {
		closeDecorate();
	}
});

inventoryTabs.forEach((tab) => {
	tab.addEventListener('click', () => {
		activeFilter = tab.dataset.filter;
		inventoryTabs.forEach((button) => {
			const isActive = button === tab;
			button.classList.toggle('is-active', isActive);
			button.setAttribute('aria-pressed', String(isActive));
		});
		renderInventory();
	});
});

shopTabs.forEach((tab) => {
	tab.addEventListener('click', () => {
		activeShopFilter = tab.dataset.shopFilter;
		shopTabs.forEach((button) => {
			const isActive = button === tab;
			button.classList.toggle('is-active', isActive);
			button.setAttribute('aria-pressed', String(isActive));
		});
		renderShop();
	});
});

shopGrid.addEventListener('click', (event) => {
	const buyButton = event.target.closest('[data-buy-id]');
	if (buyButton && !buyButton.disabled) {
		const item = shopItems.find((shopItem) => shopItem.id === Number(buyButton.dataset.buyId));
		if (item) {
			openPurchaseDialog(item);
		}
		shopMessage.textContent = '';
	}
});

// Áp nền & sàn đang trang bị lên sảnh chờ (khu vực .pet-game).
function applyLobbyDecor() {
	if (equippedDecor.background) {
		lobbyScene.dataset.scene = String(equippedDecor.background);
		lobbyScene.classList.add('is-active');
		petGame.classList.add('has-scene');
	} else {
		lobbyScene.removeAttribute('data-scene');
		lobbyScene.classList.remove('is-active');
		petGame.classList.remove('has-scene');
	}
	if (equippedDecor.floor) {
		lobbyFloor.dataset.floor = String(equippedDecor.floor);
		lobbyFloor.classList.add('is-active');
	} else {
		lobbyFloor.removeAttribute('data-floor');
		lobbyFloor.classList.remove('is-active');
	}
}

function buildDecorCard(item) {
	const id = item ? item.id : null;
	const selected = equippedDecor[activeDecorSlot] === id;
	const card = document.createElement('button');
	card.type = 'button';
	card.className = `bag-cell${selected ? ' is-selected' : ''}`;
	card.setAttribute('aria-pressed', String(selected));

	let art;
	if (item) {
		art = buildItemArt(item);
	} else {
		art = document.createElement('div');
		art.className = 'win-art art-default';
	}

	const name = document.createElement('span');
	name.className = 'bag-name';
	name.textContent = item ? item.name : 'Default room';

	const status = document.createElement('span');
	status.className = 'bag-status';
	status.textContent = selected ? 'In use' : 'Tap to use';

	card.append(art, name, status);
	card.addEventListener('click', () => chooseDecor(id));
	return card;
}

function renderDecorate() {
	decorateGrid.replaceChildren();
	const owned = inventory.filter((item) => item.category === 'decoration' && item.filter === activeDecorSlot);
	decorateHint.textContent = owned.length
		? `${owned.length} ${activeDecorSlot} item${owned.length > 1 ? 's' : ''} owned`
		: `No ${activeDecorSlot} items yet`;

	decorateGrid.append(buildDecorCard(null));
	owned.forEach((item) => decorateGrid.append(buildDecorCard(item)));

	if (owned.length === 0) {
		const empty = document.createElement('p');
		empty.className = 'win-empty';
		empty.innerHTML = `<strong>Nothing here yet</strong>Buy a ${activeDecorSlot} in the shop to decorate the lobby.`;
		decorateGrid.append(empty);
	}
}

async function chooseDecor(itemId) {
	if (!accountReady) return;
	try {
		const result = await api.equipDecoration(activeDecorSlot, itemId);
		applyAccountState(result);
	} catch (failure) {
		decorateHint.textContent = failure.message;
	}
}

function openDecorate() {
	if (!accountReady) return;
	decoratePanel.hidden = false;
	document.body.classList.add('inventory-open');
	renderDecorate();
	closeDecorateButton.focus();
}

function closeDecorate() {
	decoratePanel.hidden = true;
	document.body.classList.remove('inventory-open');
	openDecorateButton.focus();
}

openDecorateButton.addEventListener('click', openDecorate);
closeDecorateButton.addEventListener('click', closeDecorate);

decoratePanel.addEventListener('click', (event) => {
	if (event.target === decoratePanel) {
		closeDecorate();
	}
});

decorateTabs.forEach((tab) => {
	tab.addEventListener('click', () => {
		activeDecorSlot = tab.dataset.decorSlot;
		decorateTabs.forEach((button) => {
			const isActive = button === tab;
			button.classList.toggle('is-active', isActive);
			button.setAttribute('aria-pressed', String(isActive));
		});
		renderDecorate();
	});
});

renderInventory();
updateCoinDisplays();
