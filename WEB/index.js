let slideIndex = 1;
showSlides(slideIndex);
let slideTimer = setInterval(() => plusSlides(1), 3000); // Tự động chuyển sau 3 giây

// Chuyển slide khi bấm nút
function plusSlides(n) {
  clearInterval(slideTimer); // Dừng thời gian cũ
  showSlides(slideIndex += n);
  slideTimer = setInterval(() => plusSlides(1), 3000); // Khởi động lại bộ đếm
}

// Chọn slide trực tiếp
function currentSlide(n) {
  clearInterval(slideTimer);
  showSlides(slideIndex = n);
  slideTimer = setInterval(() => plusSlides(1), 3000);
}

// Hiển thị slide tương ứng
function showSlides(n) {
  let slides = document.getElementsByClassName("mySlides");
  let dots = document.getElementsByClassName("dot");
  
  if (n > slides.length) { slideIndex = 1 }
  if (n < 1) { slideIndex = slides.length }

  for (let i = 0; i < slides.length; i++) {
    slides[i].style.display = "none";
  }
  
  for (let i = 0; i < dots.length; i++) {
    dots[i].className = dots[i].className.replace(" active", "");
  }

  slides[slideIndex - 1].style.display = "block";
  dots[slideIndex - 1].className += " active";
}



//logo
document.addEventListener("DOMContentLoaded", function () {
  const logoName = document.querySelector(".logo_name a");
  const text = logoName.innerText.trim(); // Lấy nội dung chữ

  logoName.innerHTML = ""; // Xóa nội dung cũ

  text.split("").forEach((letter, index) => {
      let span = document.createElement("span");
      span.innerHTML = letter === " " ? "&nbsp;" : letter; // Giữ khoảng trắng
      logoName.appendChild(span);
  });

  function animateLetters() {
      let spans = logoName.querySelectorAll("span");
      spans.forEach((span, index) => {
          setTimeout(() => {
              span.classList.add("active");
              setTimeout(() => {
                  span.classList.remove("active");
              }, 500);
          }, index * 150); // Mỗi chữ nhảy lên cách nhau 150ms
      });

      setTimeout(animateLetters, spans.length * 150 + 500); // Lặp lại sau khi hoàn thành
  }

  animateLetters(); // Gọi hàm để bắt đầu hiệu ứng
});


