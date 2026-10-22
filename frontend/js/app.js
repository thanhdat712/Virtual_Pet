const shopItems = [];
const inventory = [];
let coins = 0;
let accountReady = false;

const inventoryPanel = document.querySelector('#inventory-panel');
const inventoryGrid = document.querySelector('#inventory-grid');
const inventoryCount = document.querySelector('#inventory-count');
const openInventoryButton = document.querySelector('#open-inventory');
const closeInventoryButton = document.querySelector('#close-inventory');
const inventoryTabs = document.querySelectorAll('.inventory-tab');
let activeFilter = 'all';
const shopPanel = document.querySelector('#shop-panel');
const shopGrid = document.querySelector('#shop-grid');
const shopMessage = document.querySelector('#shop-message');
const shopCoinBalance = document.querySelector('#shop-coin-balance');
const coinBalance = document.querySelector('#coin-balance');
const openShopButton = document.querySelector('#open-shop');
const closeShopButton = document.querySelector('#close-shop');
const shopTabs = document.querySelectorAll('.shop-tab');
let activeShopFilter = 'all';
let purchaseDialog = null;

function updateCoinDisplays() {
	coinBalance.textContent = coins;
	shopCoinBalance.textContent = coins;
}

function renderInventory() {
	const visibleItems = inventory.filter((item) => {
		return activeFilter === 'all' || item.category === activeFilter;
	});
	const totalQuantity = visibleItems.reduce((total, item) => total + item.quantity, 0);

	inventoryCount.textContent = `${visibleItems.length} types · ${totalQuantity} items`;
	inventoryGrid.replaceChildren();

	if (visibleItems.length === 0) {
		const emptyState = document.createElement('div');
		emptyState.className = 'inventory-empty';
		emptyState.innerHTML = '<strong>This section is empty</strong>Visit the shop to find something for UIA.';
		inventoryGrid.append(emptyState);
		return;
	}

	const groups = activeFilter === 'all'
		? [
			{ category: 'food', title: 'Food', items: visibleItems.filter((item) => item.category === 'food') },
			{ category: 'decoration', title: 'Decorations', items: visibleItems.filter((item) => item.category === 'decoration') }
		]
		: [{ category: activeFilter, title: activeFilter === 'food' ? 'Food' : 'Decorations', items: visibleItems }];

	groups.filter((group) => group.items.length > 0).forEach((group) => {
		const section = document.createElement('section');
		section.className = 'inventory-section';

		const heading = document.createElement('h2');
		heading.className = 'inventory-section-title';
		heading.textContent = group.title;

		const itemGrid = document.createElement('div');
		itemGrid.className = 'inventory-section-grid';

		group.items.forEach((item) => {
			const card = document.createElement('article');
			card.className = `inventory-item inventory-item-${item.category}`;

			const art = document.createElement('div');
			art.className = 'item-art';

			const image = document.createElement('img');
			image.src = item.image;
			image.alt = item.name;
			image.addEventListener('error', () => {
				const fallback = document.createElement('span');
				fallback.className = 'item-fallback';
				fallback.setAttribute('aria-hidden', 'true');
				fallback.textContent = item.icon;
				image.replaceWith(fallback);
			}, { once: true });
			art.append(image);

			const category = document.createElement('p');
			category.className = 'item-category';
			category.textContent = item.categoryLabel;

			const name = document.createElement('h3');
			name.textContent = item.name;

			const description = document.createElement('p');
			description.className = 'item-description';
			description.textContent = item.description;

			const footer = document.createElement('div');
			footer.className = 'item-footer';

			const quantity = document.createElement('span');
			quantity.className = 'item-quantity';
			quantity.textContent = `Quantity: ${item.quantity}`;

			footer.append(quantity);
			card.append(art, category, name, description, footer);
			itemGrid.append(card);
		});

		section.append(heading, itemGrid);
		inventoryGrid.append(section);
	});
}

function renderShop() {
	const visibleItems = shopItems.filter((item) => {
		return activeShopFilter === 'all' || item.filter === activeShopFilter;
	});
	shopGrid.replaceChildren();

	visibleItems.forEach((item) => {
		const card = document.createElement('article');
		const owned = item.category === 'decoration' && inventory.some((ownedItem) => ownedItem.id === item.id);
		card.className = `inventory-item shop-item inventory-item-${item.category}${owned ? ' shop-item-owned' : ''}`;

		const art = document.createElement('div');
		art.className = 'item-art';
		const image = document.createElement('img');
		image.src = item.image;
		image.alt = item.name;
		image.addEventListener('error', () => {
			const fallback = document.createElement('span');
			fallback.className = 'item-fallback';
			fallback.setAttribute('aria-hidden', 'true');
			fallback.textContent = item.icon;
			image.replaceWith(fallback);
		}, { once: true });
		art.append(image);
		if (owned) {
			const ownedBadge = document.createElement('span');
			ownedBadge.className = 'shop-owned-badge';
			ownedBadge.textContent = 'OWNED';
			art.append(ownedBadge);
		}

		const category = document.createElement('p');
		category.className = 'item-category';
		category.textContent = item.categoryLabel;

		const name = document.createElement('h3');
		name.textContent = item.name;

		const description = document.createElement('p');
		description.className = 'item-description';
		description.textContent = item.description;

		const footer = document.createElement('div');
		footer.className = 'shop-item-footer';

		const buyButton = document.createElement('button');
		buyButton.type = 'button';
		buyButton.className = 'shop-buy-button';
		buyButton.dataset.buyId = String(item.id);
		buyButton.disabled = item.category === 'decoration' && owned;
		buyButton.textContent = buyButton.disabled ? 'Owned' : 'Buy';

		const price = document.createElement('span');
		price.className = 'shop-item-price';
		price.textContent = `${item.price} coins`;

		footer.append(price, buyButton);

		card.append(art, category, name, description, footer);
		shopGrid.append(card);
	});

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
	document.body.append(backdrop);
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

renderInventory();
updateCoinDisplays();
