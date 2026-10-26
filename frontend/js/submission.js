(() => {
  const form = document.getElementById('submissionForm');
  const source = document.getElementById('sourceCode');
  const result = document.getElementById('result');
  const submitButton = document.getElementById('submitButton');
  const byteCount = document.getElementById('byteCount');
  let pollTimer = null;

  function show(value) {
    result.textContent = typeof value === 'string' ? value : JSON.stringify(value, null, 2);
  }
  function updateBytes() {
    const bytes = new TextEncoder().encode(source.value).length;
    byteCount.textContent = `${bytes} / 32768 bytes`;
    byteCount.style.color = bytes > 32768 ? '#b42318' : '';
  }
  source.addEventListener('input', updateBytes);
  updateBytes();

  async function poll(id) {
    try {
      const item = await window.api.getSubmission(id);
      show(item);
      // Do not poll forever while the evaluator is not configured.
      if (item.finished || (item.message || '').includes('not connected yet')) {
        submitButton.disabled = false;
        return;
      }
      pollTimer = window.setTimeout(() => poll(id), 1500);
    } catch (error) {
      show(`Không lấy được kết quả: ${error.message}`);
      submitButton.disabled = false;
    }
  }

  form.addEventListener('submit', async event => {
    event.preventDefault();
    if (pollTimer) window.clearTimeout(pollTimer);
    const exerciseId = Number(document.getElementById('exerciseId').value);
    const code = source.value;
    const bytes = new TextEncoder().encode(code).length;
    if (!Number.isInteger(exerciseId) || exerciseId < 1) return show('Mã bài tập không hợp lệ.');
    if (!code.trim()) return show('Vui lòng nhập mã nguồn.');
    if (bytes > 32768) return show('Mã nguồn vượt giới hạn 32768 bytes.');
    submitButton.disabled = true;
    show('Đang gửi bài...');
    try {
      const submission = await window.api.submitCode(exerciseId, code, document.getElementById('language').value);
      show(submission);
      await poll(submission.submissionId || submission.id);
    } catch (error) {
      show(`Nộp bài thất bại: ${error.message}`);
      submitButton.disabled = false;
    }
  });
})();
