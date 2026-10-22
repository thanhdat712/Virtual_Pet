// Lớp gọi API cho frontend. Backend C++ chạy tại http://127.0.0.1:8080.
// Token đăng nhập được lưu trong localStorage để giữ phiên giữa các lần mở trang.
const API_BASE = location.protocol === 'http:' ? location.origin : 'http://127.0.0.1:8080';

const auth = {
	get token() {
		return localStorage.getItem('vp_token') || '';
	},
	set token(value) {
		if (value) {
			localStorage.setItem('vp_token', value);
		} else {
			localStorage.removeItem('vp_token');
		}
	},
	get isLoggedIn() {
		return Boolean(this.token);
	},
	async logout() {
		await request('POST', '/api/logout');
		this.token = '';
	}
};

// Gửi request JSON và trả về object đã parse; ném lỗi kèm message từ server.
async function request(method, path, body) {
	const headers = { 'Content-Type': 'application/json' };
	if (auth.token) {
		headers.Authorization = `Bearer ${auth.token}`;
	}
	const response = await fetch(`${API_BASE}${path}`, {
		method,
		headers,
		body: body ? JSON.stringify(body) : undefined,
		signal: AbortSignal.timeout(15000)
	});
	const text = await response.text();
	const data = text ? JSON.parse(text) : {};
	if (!response.ok) {
		const error = new Error(data.error || `Loi ${response.status}`);
		error.status = response.status;
		if (response.status === 401) auth.token = '';
		throw error;
	}
	return data;
}

const api = {
	auth,

	async register(email, password) {
		const data = await request('POST', '/api/register', { email, password });
		auth.token = data.token;
		return data;
	},

	async login(email, password) {
		const data = await request('POST', '/api/login', { email, password });
		auth.token = data.token;
		return data;
	},

	// Lấy tài khoản + pet + tiến độ bài tập của người đang đăng nhập.
	me() {
		return request('GET', '/api/me');
	},

	// Backend chi cho phep doi ten pet; economy do server quan ly.
	savePet(petState) {
		return request('PUT', '/api/pet', petState);
	},

	// Danh sách bài tập (công khai).
	listExercises() {
		return request('GET', '/api/exercises');
	},
	listShop() {
		return request('GET', '/api/shop');
	},
	buyItem(itemId, quantity) {
		return request('POST', '/api/shop/purchases', { itemId, quantity });
	},
	feedPet(itemId) {
		return request('POST', '/api/pet/feed', { itemId });
	},

	// Gui dap an; backend cham diem va luu tien do theo tai khoan.
	submitExercise(exerciseId, answer) {
		return request('POST', `/api/exercises/${exerciseId}/submissions`, { answer });
	}
};

window.api = api;
