#ifndef BYTEARRAYSERIALIZER_H
#define BYTEARRAYSERIALIZER_H


class ByteArraySerializer
{
public:
	ByteArraySerializer(QByteArray *ba): ba_(ba){};
	template<typename T>
	ByteArraySerializer add(T t){
		static_assert(std::is_standard_layout<T>::value && std::is_trivial<T>::value, "only for pods");
		ba_->append(reinterpret_cast<const char *>(&t), sizeof(T));
		return *this;
	}
private:
	QByteArray *ba_;
};

class ByteArrayDeSerializer
{
public:
	ByteArrayDeSerializer(QByteArray *ba): ba_(ba){};
	// doesn't do any validity check on serilized data
	template<typename T>
	ByteArrayDeSerializer get(T &t){
		static_assert(std::is_standard_layout<T>::value && std::is_trivial<T>::value, "only for pods");
		char *dst = reinterpret_cast<char*>(&t);
		const char *src = ba_->data() + ndx_;
		ASSERT(ba_->size() >= ndx_ + (int) sizeof(T));
		for (size_t i = 0; i < sizeof(T); i++)
			dst[i] = src[i];
		ndx_ += sizeof(T);
		return *this;
	}
private:
	QByteArray *ba_;
	int         ndx_ = 0;
};

#endif // BYTEARRAYSERIALIZER_H
