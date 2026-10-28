let petLevel = 0;
const petStage = document.querySelector('.pet-stage');
const sleepingGameScene = document.querySelector('.pet-game');
const sleepButton = document.querySelector('#sleep-pet');
const feedingStatus = document.querySelector('#feeding-status');
let feedingStatusTimeout;
let sleepCountdownTimer;
let petSleeping = false;

function applyAccountState(state) {
	coins = state.user.coins;
	if (state.inventory) {
		inventory.splice(0, inventory.length, ...state.inventory.map((owned) => {
			const itemId = owned.itemId ?? owned.id;
			return { ...shopItems.find((item) => item.id === itemId), id: itemId, quantity: owned.quantity };
		}));
	}
	if (state.pet) {
		const petName = state.pet.name;
		const species = state.pet.species === 'dog' ? 'dog' : 'cat';
		window.currentPetProfile = { name: petName, species, speciesChosen: state.pet.speciesChosen !== false };
		document.querySelector('#pet-name').textContent = petName;
		document.querySelector('.pet-stage .pet-name').textContent = petName;
		const petImage = species === 'dog' ? 'assets/pets/gaugau.png' : 'assets/pets/meomeo.png';
		document.querySelector('.pet-stage .pet-image').src = petImage;
		document.querySelector('.pet-stage .pet-image').alt = `${species === 'dog' ? 'Dog' : 'Cat'}, ${petName}`;
		document.querySelector('.hud-avatar img').src = petImage;
		document.querySelector('#pet-level').textContent = state.pet.level;
		petLevel = state.pet.level;
		const exp = state.pet.exp ?? state.pet.experience ?? 0;
		const expMax = state.pet.level * 100;
		document.querySelector('#exp-text').textContent = `EXP ${exp} / ${expMax}`;
		document.querySelector('#exp-fill').style.width = `${Math.min(100, exp / expMax * 100)}%`;
		['happiness', 'hunger', 'energy'].forEach((name) => {
			const stat = document.querySelector(`[data-stat="${name}"]`);
			stat.querySelector('.hud-stat-value').textContent = `${state.pet[name]} / 100`;
			stat.querySelector('.px-bar i').style.width = `${state.pet[name]}%`;
		});
		syncSleepState(state.pet.sleepRemainingSeconds);
	}
	const equipment = state.equipment ?? state.decor;
	if (equipment) {
		equippedDecor.background = equipment.background ?? null;
		equippedDecor.floor = equipment.floor ?? null;
		equippedDecor.headwear = equipment.headwear ?? null;
	}
	if (typeof state.settings?.soundEnabled === 'boolean') {
		window.applySoundPreference?.(state.settings.soundEnabled);
	}
	accountReady = true;
	updateCoinDisplays();
	renderInventory();
	renderShop();
	renderFood();
	applyLobbyDecor();
	renderDecorate();
}

async function loadAccount() {
	const catalog = await api.listShop();
	shopItems.splice(0, shopItems.length, ...catalog);
	applyAccountState(await api.gameState());
}

function reportFailure(failure) {
	return failure.message || 'Unable to update the demo.';
}

document.querySelector('#open-food').addEventListener('click', () => {
	if (!accountReady) return;
	const drawer = document.querySelector('#food-drawer');
	drawer.hidden = !drawer.hidden;
	document.querySelector('#open-food').setAttribute('aria-expanded', String(!drawer.hidden));
	renderFood();
});

let feeding = false;
let selectedFood = null;

function setFeedingStatus(message, duration = 0) {
	clearTimeout(feedingStatusTimeout);
	feedingStatusTimeout = null;
	feedingStatus.textContent = message;
	if (duration > 0) {
		feedingStatusTimeout = setTimeout(() => {
			feedingStatus.textContent = '';
			feedingStatusTimeout = null;
		}, duration);
	}
}

function syncSleepState(remainingSeconds = 0) {
	window.clearInterval(sleepCountdownTimer);
	sleepCountdownTimer = null;
	const remaining = Math.max(0, Math.floor(Number(remainingSeconds) || 0));
	petSleeping = remaining > 0;
	petStage.classList.toggle('is-sleeping', petSleeping);
	sleepingGameScene.classList.toggle('is-sleeping', petSleeping);
	sleepButton.disabled = petSleeping;
	if (!petSleeping) return;
	setFeedingStatus('');

	const wakeAt = Date.now() + remaining * 1000;
	const updateCountdown = () => {
		const secondsLeft = Math.max(0, Math.ceil((wakeAt - Date.now()) / 1000));
		if (secondsLeft === 0) {
			window.clearInterval(sleepCountdownTimer);
			sleepCountdownTimer = null;
			loadAccount().then(() => {
				if (!petSleeping) setFeedingStatus(`${window.getCurrentPetName()} woke up! Energy increased by 1.`);
			}).catch(() => {
				setFeedingStatus('Could not refresh your pet after sleep. Reconnecting...');
				syncSleepState(1);
			});
			return;
		}
	};
	updateCountdown();
	sleepCountdownTimer = window.setInterval(updateCountdown, 1000);
}

petStage.tabIndex = 0;
petStage.setAttribute('role', 'button');
petStage.setAttribute('aria-label', `Feed selected food to ${window.getCurrentPetName()}`);

document.querySelector('#close-food').addEventListener('click', () => {
	document.querySelector('#food-drawer').hidden = true;
	document.querySelector('#open-food').setAttribute('aria-expanded', 'false');
});

function renderFood() {
	const list = document.querySelector('#food-list');
	list.replaceChildren();
	const foods = inventory.filter((item) => item.category === 'food' && item.quantity > 0);
	if (!foods.length) {
		const empty = document.createElement('p');
		empty.textContent = 'No food in inventory';
		list.append(empty);
	}
	foods.forEach((item) => {
		const button = document.createElement('button');
		button.type = 'button';
		button.className = 'food-choice';
		button.disabled = feeding;
		button.setAttribute('aria-label', `${item.name}, ${item.quantity} remaining`);
		button.setAttribute('aria-pressed', String(selectedFood === item.id));
		const image = document.createElement('img');
		image.src = item.image;
		image.alt = '';
		image.draggable = false;
		const label = document.createElement('span');
		label.textContent = item.name;
		const count = document.createElement('b');
		count.textContent = `\u00d7${item.quantity}`;
		button.append(image, label, count);
		button.addEventListener('click', () => {
			selectedFood = item.id;
			list.querySelectorAll('button').forEach((choice) => choice.setAttribute('aria-pressed', String(choice === button)));
		});
		button.addEventListener('pointerdown', (event) => {
			if (feeding || event.button !== 0) return;
			const ghost = image.cloneNode();
			ghost.className = 'food-drag-ghost';
			document.body.append(ghost);
			button.setPointerCapture(event.pointerId);
			const move = (pointer) => {
				ghost.style.left = `${pointer.clientX}px`;
				ghost.style.top = `${pointer.clientY}px`;
				const bounds = petStage.getBoundingClientRect();
				petStage.classList.toggle('is-drop-target', pointer.clientX >= bounds.left && pointer.clientX <= bounds.right && pointer.clientY >= bounds.top && pointer.clientY <= bounds.bottom);
			};
			move(event);
			const finish = (pointer) => {
				const dropped = petStage.classList.contains('is-drop-target');
				ghost.remove();
				petStage.classList.remove('is-drop-target');
				button.removeEventListener('pointermove', move);
				button.removeEventListener('pointerup', finish);
				button.removeEventListener('pointercancel', finish);
				if (pointer.type === 'pointerup' && dropped) feedFood(item);
			};
			button.addEventListener('pointermove', move);
			button.addEventListener('pointerup', finish);
			button.addEventListener('pointercancel', finish);
		});
		list.append(button);
	});
}

async function feedFood(item) {
	if (feeding || !accountReady || !item) return;
	feeding = true;
	setFeedingStatus('');
	renderFood();
	try {
		const previousLevel = petLevel;
		const fed = await api.feedPet(item.id);
		window.playGameSound?.('eat');
		applyAccountState(fed);
		petStage.classList.remove('is-eating');
		void petStage.offsetWidth;
		petStage.classList.add('is-eating');
		const bite = document.createElement('img');
		bite.src = item.image;
		bite.alt = '';
		bite.className = 'eating-food';
		petStage.append(bite);
		bite.addEventListener('animationend', () => bite.remove(), { once: true });
		setFeedingStatus(fed.pet.level > previousLevel
			? `${item.name} eaten! Level up! ${window.getCurrentPetName()} is now level ${fed.pet.level}`
			: `${item.name} eaten!`);
	} catch (failure) {
		setFeedingStatus(reportFailure(failure));
	} finally {
		feeding = false;
		renderFood();
	}
}

petStage.addEventListener('animationend', () => petStage.classList.remove('is-eating'));
petStage.addEventListener('click', () => feedFood(inventory.find((item) => item.id === selectedFood)));
petStage.addEventListener('keydown', (event) => {
	if (event.key === 'Enter' || event.key === ' ') {
		event.preventDefault();
		feedFood(inventory.find((item) => item.id === selectedFood));
	}
});

document.querySelector('#sleep-pet').addEventListener('click', async (event) => {
	if (!accountReady) return;
	const button = event.currentTarget;
	button.disabled = true;
	try {
		applyAccountState(await api.sleepPet());
		window.playGameSound?.('sleep');
	} catch (failure) {
		const message = reportFailure(failure);
		setFeedingStatus(message, message.includes('not sleepy right now') ? 5000 : 0);
	} finally {
		button.disabled = petSleeping;
	}
});

async function initializeAccount() {
	try {
		await loadAccount();
		return true;
	} catch (failure) {
		setFeedingStatus(reportFailure(failure));
		return false;
	}
}

window.initializeAccount = initializeAccount;
